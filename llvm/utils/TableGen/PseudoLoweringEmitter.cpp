//===- PseudoLoweringEmitter.cpp - PseudoLowering Generator -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Common/CodeGenInstruction.h"
#include "Common/CodeGenTarget.h"
#include "llvm/ADT/IndexedMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/TableGen/Error.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TGTimer.h"
#include "llvm/TableGen/TableGenBackend.h"
using namespace llvm;

#define DEBUG_TYPE "pseudo-lowering"

namespace {
class PseudoLoweringEmitter {
  struct OpData {
    enum MapKind { Operand, Imm, Reg };
    MapKind Kind;
    union {
      unsigned Operand; // Operand number mapped to.
      uint64_t Imm;     // Integer immedate value.
      const Record *Reg; // Physical register.
    } Data;
  };
  struct PseudoExpansion {
    CodeGenInstruction Source; // The source pseudo instruction definition.
    CodeGenInstruction Dest;   // The destination instruction to lower to.
    IndexedMap<OpData> OperandMap;

    PseudoExpansion(CodeGenInstruction &s, CodeGenInstruction &d,
                    IndexedMap<OpData> &m)
        : Source(s), Dest(d), OperandMap(m) {}
  };

  const RecordKeeper &Records;

  // It's overkill to have an instance of the full CodeGenTarget object,
  // but it loads everything on demand, not in the constructor, so it's
  // lightweight in performance, so it works out OK.
  const CodeGenTarget Target;

  SmallVector<PseudoExpansion, 64> Expansions;

  unsigned addDagOperandMapping(const Record *Rec, const DagInit *Dag,
                                const CodeGenInstruction &Insn,
                                IndexedMap<OpData> &OperandMap,
                                unsigned BaseIdx);
  void evaluateExpansion(const Record *Pseudo);
  void emitLoweringEmitter(raw_ostream &o);

public:
  PseudoLoweringEmitter(const RecordKeeper &R) : Records(R), Target(R) {}

  /// run - Output the pseudo-lowerings.
  void run(raw_ostream &o);
};
} // End anonymous namespace

// FIXME: This pass currently can only expand a pseudo to a single instruction.
//        The pseudo expansion really should take a list of dags, not just
//        a single dag, so we can do fancier things.
unsigned PseudoLoweringEmitter::addDagOperandMapping(
    const Record *Rec, const DagInit *Dag, const CodeGenInstruction &Insn,
    IndexedMap<OpData> &OperandMap, unsigned BaseIdx) {
  unsigned OpsAdded = 0;
  for (unsigned i = 0, e = Dag->getNumArgs(); i != e; ++i) {
    if (const DefInit *DI = dyn_cast<DefInit>(Dag->getArg(i))) {
      // Physical register reference. Explicit check for the special case
      // "zero_reg" definition.
      if (DI->getDef()->isSubClassOf("Register") ||
          DI->getDef()->getName() == "zero_reg") {
        auto &Entry = OperandMap[BaseIdx + i];
        Entry.Kind = OpData::Reg;
        Entry.Data.Reg = DI->getDef();
        ++OpsAdded;
        continue;
      }

      // Normal operands should always have the same type, or we have a
      // problem.
      // FIXME: We probably shouldn't ever get a non-zero BaseIdx here.
      assert(BaseIdx == 0 && "Named subargument in pseudo expansion?!");
      if (DI->getDef() != Insn.Operands[BaseIdx + i].Rec) {
        PrintError(Rec, "In pseudo instruction '" + Rec->getName() +
                            "', operand type '" + DI->getDef()->getName() +
                            "' does not match expansion operand type '" +
                            Insn.Operands[BaseIdx + i].Rec->getName() + "'");
        PrintFatalNote(DI->getDef(),
                       "Value was assigned at the following location:");
      }
      // Source operand maps to destination operand. The Data element
      // will be filled in later, just set the Kind for now. Do it
      // for each corresponding MachineInstr operand, not just the first.
      for (unsigned I = 0, E = Insn.Operands[i].MINumOperands; I != E; ++I)
        OperandMap[BaseIdx + i + I].Kind = OpData::Operand;
      OpsAdded += Insn.Operands[i].MINumOperands;
    } else if (const IntInit *II = dyn_cast<IntInit>(Dag->getArg(i))) {
      auto &Entry = OperandMap[BaseIdx + i];
      Entry.Kind = OpData::Imm;
      Entry.Data.Imm = II->getValue();
      ++OpsAdded;
    } else if (const auto *BI = dyn_cast<BitsInit>(Dag->getArg(i))) {
      auto &Entry = OperandMap[BaseIdx + i];
      Entry.Kind = OpData::Imm;
      Entry.Data.Imm = *BI->convertInitializerToInt();
      ++OpsAdded;
    } else if (const DagInit *SubDag = dyn_cast<DagInit>(Dag->getArg(i))) {
      // Just add the operands recursively. This is almost certainly
      // a constant value for a complex operand (> 1 MI operand).
      unsigned NewOps =
          addDagOperandMapping(Rec, SubDag, Insn, OperandMap, BaseIdx + i);
      OpsAdded += NewOps;
      // Since we added more than one, we also need to adjust the base.
      BaseIdx += NewOps - 1;
    } else
      llvm_unreachable("Unhandled pseudo-expansion argument type!");
  }
  return OpsAdded;
}

void PseudoLoweringEmitter::evaluateExpansion(const Record *Rec) {
  LLVM_DEBUG(dbgs() << "Pseudo definition: " << Rec->getName() << "\n");

  // Validate that the result pattern has the corrent number and types
  // of arguments for the instruction it references.
  const DagInit *Dag = Rec->getValueAsDag("ResultInst");
  assert(Dag && "Missing result instruction in pseudo expansion!");
  LLVM_DEBUG(dbgs() << "  Result: " << *Dag << "\n");

  const DefInit *OpDef = dyn_cast<DefInit>(Dag->getOperator());
  if (!OpDef) {
    PrintError(Rec, "In pseudo instruction '" + Rec->getName() +
                        "', result operator is not a record");
    PrintFatalNote(Rec->getValue("ResultInst"),
                   "Result was assigned at the following location:");
  }
  const Record *Operator = OpDef->getDef();
  if (!Operator->isSubClassOf("Instruction")) {
    PrintError(Rec, "In pseudo instruction '" + Rec->getName() +
                        "', result operator '" + Operator->getName() +
                        "' is not an instruction");
    PrintFatalNote(Rec->getValue("ResultInst"),
                   "Result was assigned at the following location:");
  }

  CodeGenInstruction Insn(Operator);

  if (Insn.isCodeGenOnly || Insn.isPseudo) {
    PrintError(Rec, "In pseudo instruction '" + Rec->getName() +
                        "', result operator '" + Operator->getName() +
                        "' cannot be a pseudo instruction");
    PrintFatalNote(Rec->getValue("ResultInst"),
                   "Result was assigned at the following location:");
  }

  if (Insn.Operands.size() != Dag->getNumArgs()) {
    PrintError(Rec, "In pseudo instruction '" + Rec->getName() +
                        "', result operator '" + Operator->getName() +
                        "' has the wrong number of operands");
    PrintFatalNote(Rec->getValue("ResultInst"),
                   "Result was assigned at the following location:");
  }

  unsigned NumMIOperands = 0;
  for (const auto &Op : Insn.Operands)
    NumMIOperands += Op.MINumOperands;
  IndexedMap<OpData> OperandMap;
  OperandMap.grow(NumMIOperands);

  addDagOperandMapping(Rec, Dag, Insn, OperandMap, 0);

  // If there are more operands that weren't in the DAG, they have to
  // be operands that have default values, or we have an error. Currently,
  // Operands that are a subclass of OperandWithDefaultOp have default values.

  // Validate that each result pattern argument has a matching (by name)
  // argument in the source instruction, in either the (outs) or (ins) list.
  // Also check that the type of the arguments match.
  //
  // Record the mapping of the source to result arguments for use by
  // the lowering emitter.
  CodeGenInstruction SourceInsn(Rec);
  StringMap<unsigned> SourceOperands;
  for (const auto &[Idx, SrcOp] : enumerate(SourceInsn.Operands))
    SourceOperands[SrcOp.Name] = Idx;

  LLVM_DEBUG(dbgs() << "  Operand mapping:\n");
  for (unsigned i = 0, e = Insn.Operands.size(); i != e; ++i) {
    // We've already handled constant values. Just map instruction operands
    // here.
    if (OperandMap[Insn.Operands[i].MIOperandNo].Kind != OpData::Operand)
      continue;
    StringMap<unsigned>::iterator SourceOp =
        SourceOperands.find(Dag->getArgNameStr(i));
    if (SourceOp == SourceOperands.end()) {
      PrintError(Rec, "In pseudo instruction '" + Rec->getName() +
                          "', output operand '" + Dag->getArgNameStr(i) +
                          "' has no matching source operand");
      PrintFatalNote(Rec->getValue("ResultInst"),
                     "Value was assigned at the following location:");
    }
    // Map the source operand to the destination operand index for each
    // MachineInstr operand.
    for (unsigned I = 0, E = Insn.Operands[i].MINumOperands; I != E; ++I)
      OperandMap[Insn.Operands[i].MIOperandNo + I].Data.Operand =
          SourceOp->getValue();

    LLVM_DEBUG(dbgs() << "    " << SourceOp->getValue() << " ==> " << i
                      << "\n");
  }

  Expansions.push_back(PseudoExpansion(SourceInsn, Insn, OperandMap));
}

void PseudoLoweringEmitter::emitLoweringEmitter(raw_ostream &o) {
  // Emit file header.
  emitSourceFileHeader("Pseudo-instruction MC lowering Source Fragment", o);

  o << "bool " << Target.getName() + "AsmPrinter::\n"
    << "lowerPseudoInstExpansion(const MachineInstr *MI, MCInst &Inst) {\n";

  if (!Expansions.empty()) {
    o << "  Inst.clear();\n"
      << "  switch (MI->getOpcode()) {\n"
      << "  default: return false;\n";
    for (auto &Expansion : Expansions) {
      CodeGenInstruction &Source = Expansion.Source;
      CodeGenInstruction &Dest = Expansion.Dest;
      o << "  case " << Source.Namespace << "::" << Source.TheDef->getName()
        << ": {\n"
        << "    MCOperand MCOp;\n"
        << "    Inst.setOpcode(" << Dest.Namespace
        << "::" << Dest.TheDef->getName() << ");\n";

      // Copy the operands from the source instruction.
      // FIXME: Instruction operands with defaults values (predicates and cc_out
      //        in ARM, for example shouldn't need explicit values in the
      //        expansion DAG.
      unsigned MIOpNo = 0;
      for (const auto &DestOperand : Dest.Operands) {
        o << "    // Operand: " << DestOperand.Name << "\n";
        for (unsigned i = 0, e = DestOperand.MINumOperands; i != e; ++i) {
          switch (Expansion.OperandMap[MIOpNo + i].Kind) {
          case OpData::Operand:
            o << "    lowerOperand(MI->getOperand("
              << Source.Operands[Expansion.OperandMap[MIOpNo].Data.Operand]
                         .MIOperandNo +
                     i
              << "), MCOp);\n"
              << "    Inst.addOperand(MCOp);\n";
            break;
          case OpData::Imm:
            o << "    Inst.addOperand(MCOperand::createImm("
              << Expansion.OperandMap[MIOpNo + i].Data.Imm << "));\n";
            break;
          case OpData::Reg: {
            const Record *Reg = Expansion.OperandMap[MIOpNo + i].Data.Reg;
            o << "    Inst.addOperand(MCOperand::createReg(";
            // "zero_reg" is special.
            if (Reg->getName() == "zero_reg")
              o << "0";
            else
              o << Reg->getValueAsString("Namespace") << "::" << Reg->getName();
            o << "));\n";
            break;
          }
          }
        }
        MIOpNo += DestOperand.MINumOperands;
      }
      if (Dest.Operands.isVariadic) {
        MIOpNo = Source.Operands.size() + 1;
        o << "    // variable_ops\n";
        o << "    for (unsigned i = " << MIOpNo
          << ", e = MI->getNumOperands(); i != e; ++i)\n"
          << "      if (lowerOperand(MI->getOperand(i), MCOp))\n"
          << "        Inst.addOperand(MCOp);\n";
      }
      o << "    break;\n"
        << "  }\n";
    }
    o << "  }\n  return true;";
  } else
    o << "  return false;";

  o << "\n}\n\n";
}

void PseudoLoweringEmitter::run(raw_ostream &OS) {
  StringRef Classes[] = {"PseudoInstExpansion", "Instruction"};

  // Process the pseudo expansion definitions, validating them as we do so.
  TGTimer &Timer = Records.getTimer();
  Timer.startTimer("Process definitions");
  for (const Record *Inst : Records.getAllDerivedDefinitions(Classes))
    evaluateExpansion(Inst);

  // Generate expansion code to lower the pseudo to an MCInst of the real
  // instruction.
  Timer.startTimer("Emit expansion code");
  emitLoweringEmitter(OS);
}

static TableGen::Emitter::OptClass<PseudoLoweringEmitter>
    X("gen-pseudo-lowering", "Generate pseudo instruction lowering");
