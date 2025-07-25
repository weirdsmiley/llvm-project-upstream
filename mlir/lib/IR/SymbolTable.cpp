//===- SymbolTable.cpp - MLIR Symbol Table Class --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/OpImplementation.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSwitch.h"
#include <optional>

using namespace mlir;

/// Return true if the given operation is unknown and may potentially define a
/// symbol table.
static bool isPotentiallyUnknownSymbolTable(Operation *op) {
  return op->getNumRegions() == 1 && !op->getDialect();
}

/// Returns the string name of the given symbol, or null if this is not a
/// symbol.
static StringAttr getNameIfSymbol(Operation *op) {
  return op->getAttrOfType<StringAttr>(SymbolTable::getSymbolAttrName());
}
static StringAttr getNameIfSymbol(Operation *op, StringAttr symbolAttrNameId) {
  return op->getAttrOfType<StringAttr>(symbolAttrNameId);
}

/// Computes the nested symbol reference attribute for the symbol 'symbolName'
/// that are usable within the symbol table operations from 'symbol' as far up
/// to the given operation 'within', where 'within' is an ancestor of 'symbol'.
/// Returns success if all references up to 'within' could be computed.
static LogicalResult
collectValidReferencesFor(Operation *symbol, StringAttr symbolName,
                          Operation *within,
                          SmallVectorImpl<SymbolRefAttr> &results) {
  assert(within->isAncestor(symbol) && "expected 'within' to be an ancestor");
  MLIRContext *ctx = symbol->getContext();

  auto leafRef = FlatSymbolRefAttr::get(symbolName);
  results.push_back(leafRef);

  // Early exit for when 'within' is the parent of 'symbol'.
  Operation *symbolTableOp = symbol->getParentOp();
  if (within == symbolTableOp)
    return success();

  // Collect references until 'symbolTableOp' reaches 'within'.
  SmallVector<FlatSymbolRefAttr, 1> nestedRefs(1, leafRef);
  StringAttr symbolNameId =
      StringAttr::get(ctx, SymbolTable::getSymbolAttrName());
  do {
    // Each parent of 'symbol' should define a symbol table.
    if (!symbolTableOp->hasTrait<OpTrait::SymbolTable>())
      return failure();
    // Each parent of 'symbol' should also be a symbol.
    StringAttr symbolTableName = getNameIfSymbol(symbolTableOp, symbolNameId);
    if (!symbolTableName)
      return failure();
    results.push_back(SymbolRefAttr::get(symbolTableName, nestedRefs));

    symbolTableOp = symbolTableOp->getParentOp();
    if (symbolTableOp == within)
      break;
    nestedRefs.insert(nestedRefs.begin(),
                      FlatSymbolRefAttr::get(symbolTableName));
  } while (true);
  return success();
}

/// Walk all of the operations within the given set of regions, without
/// traversing into any nested symbol tables. Stops walking if the result of the
/// callback is anything other than `WalkResult::advance`.
static std::optional<WalkResult>
walkSymbolTable(MutableArrayRef<Region> regions,
                function_ref<std::optional<WalkResult>(Operation *)> callback) {
  SmallVector<Region *, 1> worklist(llvm::make_pointer_range(regions));
  while (!worklist.empty()) {
    for (Operation &op : worklist.pop_back_val()->getOps()) {
      std::optional<WalkResult> result = callback(&op);
      if (result != WalkResult::advance())
        return result;

      // If this op defines a new symbol table scope, we can't traverse. Any
      // symbol references nested within 'op' are different semantically.
      if (!op.hasTrait<OpTrait::SymbolTable>()) {
        for (Region &region : op.getRegions())
          worklist.push_back(&region);
      }
    }
  }
  return WalkResult::advance();
}

/// Walk all of the operations nested under, and including, the given operation,
/// without traversing into any nested symbol tables. Stops walking if the
/// result of the callback is anything other than `WalkResult::advance`.
static std::optional<WalkResult>
walkSymbolTable(Operation *op,
                function_ref<std::optional<WalkResult>(Operation *)> callback) {
  std::optional<WalkResult> result = callback(op);
  if (result != WalkResult::advance() || op->hasTrait<OpTrait::SymbolTable>())
    return result;
  return walkSymbolTable(op->getRegions(), callback);
}

//===----------------------------------------------------------------------===//
// SymbolTable
//===----------------------------------------------------------------------===//

/// Build a symbol table with the symbols within the given operation.
SymbolTable::SymbolTable(Operation *symbolTableOp)
    : symbolTableOp(symbolTableOp) {
  assert(symbolTableOp->hasTrait<OpTrait::SymbolTable>() &&
         "expected operation to have SymbolTable trait");
  assert(symbolTableOp->getNumRegions() == 1 &&
         "expected operation to have a single region");
  assert(symbolTableOp->getRegion(0).hasOneBlock() &&
         "expected operation to have a single block");

  StringAttr symbolNameId = StringAttr::get(symbolTableOp->getContext(),
                                            SymbolTable::getSymbolAttrName());
  for (auto &op : symbolTableOp->getRegion(0).front()) {
    StringAttr name = getNameIfSymbol(&op, symbolNameId);
    if (!name)
      continue;

    auto inserted = symbolTable.insert({name, &op});
    (void)inserted;
    assert(inserted.second &&
           "expected region to contain uniquely named symbol operations");
  }
}

/// Look up a symbol with the specified name, returning null if no such name
/// exists. Names never include the @ on them.
Operation *SymbolTable::lookup(StringRef name) const {
  return lookup(StringAttr::get(symbolTableOp->getContext(), name));
}
Operation *SymbolTable::lookup(StringAttr name) const {
  return symbolTable.lookup(name);
}

void SymbolTable::remove(Operation *op) {
  StringAttr name = getNameIfSymbol(op);
  assert(name && "expected valid 'name' attribute");
  assert(op->getParentOp() == symbolTableOp &&
         "expected this operation to be inside of the operation with this "
         "SymbolTable");

  auto it = symbolTable.find(name);
  if (it != symbolTable.end() && it->second == op)
    symbolTable.erase(it);
}

void SymbolTable::erase(Operation *symbol) {
  remove(symbol);
  symbol->erase();
}

// TODO: Consider if this should be renamed to something like insertOrUpdate
/// Insert a new symbol into the table and associated operation if not already
/// there and rename it as necessary to avoid collisions. Return the name of
/// the symbol after insertion as attribute.
StringAttr SymbolTable::insert(Operation *symbol, Block::iterator insertPt) {
  // The symbol cannot be the child of another op and must be the child of the
  // symbolTableOp after this.
  //
  // TODO: consider if SymbolTable's constructor should behave the same.
  if (!symbol->getParentOp()) {
    auto &body = symbolTableOp->getRegion(0).front();
    if (insertPt == Block::iterator()) {
      insertPt = Block::iterator(body.end());
    } else {
      assert((insertPt == body.end() ||
              insertPt->getParentOp() == symbolTableOp) &&
             "expected insertPt to be in the associated module operation");
    }
    // Insert before the terminator, if any.
    if (insertPt == Block::iterator(body.end()) && !body.empty() &&
        std::prev(body.end())->hasTrait<OpTrait::IsTerminator>())
      insertPt = std::prev(body.end());

    body.getOperations().insert(insertPt, symbol);
  }
  assert(symbol->getParentOp() == symbolTableOp &&
         "symbol is already inserted in another op");

  // Add this symbol to the symbol table, uniquing the name if a conflict is
  // detected.
  StringAttr name = getSymbolName(symbol);
  if (symbolTable.insert({name, symbol}).second)
    return name;
  // If the symbol was already in the table, also return.
  if (symbolTable.lookup(name) == symbol)
    return name;

  MLIRContext *context = symbol->getContext();
  SmallString<128> nameBuffer = generateSymbolName<128>(
      name.getValue(),
      [&](StringRef candidate) {
        return !symbolTable
                    .insert({StringAttr::get(context, candidate), symbol})
                    .second;
      },
      uniquingCounter);
  setSymbolName(symbol, nameBuffer);
  return getSymbolName(symbol);
}

LogicalResult SymbolTable::rename(StringAttr from, StringAttr to) {
  Operation *op = lookup(from);
  return rename(op, to);
}

LogicalResult SymbolTable::rename(Operation *op, StringAttr to) {
  StringAttr from = getNameIfSymbol(op);
  (void)from;

  assert(from && "expected valid 'name' attribute");
  assert(op->getParentOp() == symbolTableOp &&
         "expected this operation to be inside of the operation with this "
         "SymbolTable");
  assert(lookup(from) == op && "current name does not resolve to op");
  assert(lookup(to) == nullptr && "new name already exists");

  if (failed(SymbolTable::replaceAllSymbolUses(op, to, getOp())))
    return failure();

  // Remove op with old name, change name, add with new name. The order is
  // important here due to how `remove` and `insert` rely on the op name.
  remove(op);
  setSymbolName(op, to);
  insert(op);

  assert(lookup(to) == op && "new name does not resolve to renamed op");
  assert(lookup(from) == nullptr && "old name still exists");

  return success();
}

LogicalResult SymbolTable::rename(StringAttr from, StringRef to) {
  auto toAttr = StringAttr::get(getOp()->getContext(), to);
  return rename(from, toAttr);
}

LogicalResult SymbolTable::rename(Operation *op, StringRef to) {
  auto toAttr = StringAttr::get(getOp()->getContext(), to);
  return rename(op, toAttr);
}

FailureOr<StringAttr>
SymbolTable::renameToUnique(StringAttr oldName,
                            ArrayRef<SymbolTable *> others) {

  // Determine new name that is unique in all symbol tables.
  StringAttr newName;
  {
    MLIRContext *context = oldName.getContext();
    SmallString<64> prefix = oldName.getValue();
    int uniqueId = 0;
    prefix.push_back('_');
    while (true) {
      newName = StringAttr::get(context, prefix + Twine(uniqueId++));
      auto lookupNewName = [&](SymbolTable *st) { return st->lookup(newName); };
      if (!lookupNewName(this) && llvm::none_of(others, lookupNewName)) {
        break;
      }
    }
  }

  // Apply renaming.
  if (failed(rename(oldName, newName)))
    return failure();
  return newName;
}

FailureOr<StringAttr>
SymbolTable::renameToUnique(Operation *op, ArrayRef<SymbolTable *> others) {
  StringAttr from = getNameIfSymbol(op);
  assert(from && "expected valid 'name' attribute");
  return renameToUnique(from, others);
}

/// Returns the name of the given symbol operation.
StringAttr SymbolTable::getSymbolName(Operation *symbol) {
  StringAttr name = getNameIfSymbol(symbol);
  assert(name && "expected valid symbol name");
  return name;
}

/// Sets the name of the given symbol operation.
void SymbolTable::setSymbolName(Operation *symbol, StringAttr name) {
  symbol->setAttr(getSymbolAttrName(), name);
}

/// Returns the visibility of the given symbol operation.
SymbolTable::Visibility SymbolTable::getSymbolVisibility(Operation *symbol) {
  // If the attribute doesn't exist, assume public.
  StringAttr vis = symbol->getAttrOfType<StringAttr>(getVisibilityAttrName());
  if (!vis)
    return Visibility::Public;

  // Otherwise, switch on the string value.
  return StringSwitch<Visibility>(vis.getValue())
      .Case("private", Visibility::Private)
      .Case("nested", Visibility::Nested)
      .Case("public", Visibility::Public);
}
/// Sets the visibility of the given symbol operation.
void SymbolTable::setSymbolVisibility(Operation *symbol, Visibility vis) {
  MLIRContext *ctx = symbol->getContext();

  // If the visibility is public, just drop the attribute as this is the
  // default.
  if (vis == Visibility::Public) {
    symbol->removeAttr(StringAttr::get(ctx, getVisibilityAttrName()));
    return;
  }

  // Otherwise, update the attribute.
  assert((vis == Visibility::Private || vis == Visibility::Nested) &&
         "unknown symbol visibility kind");

  StringRef visName = vis == Visibility::Private ? "private" : "nested";
  symbol->setAttr(getVisibilityAttrName(), StringAttr::get(ctx, visName));
}

/// Returns the nearest symbol table from a given operation `from`. Returns
/// nullptr if no valid parent symbol table could be found.
Operation *SymbolTable::getNearestSymbolTable(Operation *from) {
  assert(from && "expected valid operation");
  if (isPotentiallyUnknownSymbolTable(from))
    return nullptr;

  while (!from->hasTrait<OpTrait::SymbolTable>()) {
    from = from->getParentOp();

    // Check that this is a valid op and isn't an unknown symbol table.
    if (!from || isPotentiallyUnknownSymbolTable(from))
      return nullptr;
  }
  return from;
}

/// Walks all symbol table operations nested within, and including, `op`. For
/// each symbol table operation, the provided callback is invoked with the op
/// and a boolean signifying if the symbols within that symbol table can be
/// treated as if all uses are visible. `allSymUsesVisible` identifies whether
/// all of the symbol uses of symbols within `op` are visible.
void SymbolTable::walkSymbolTables(
    Operation *op, bool allSymUsesVisible,
    function_ref<void(Operation *, bool)> callback) {
  bool isSymbolTable = op->hasTrait<OpTrait::SymbolTable>();
  if (isSymbolTable) {
    SymbolOpInterface symbol = dyn_cast<SymbolOpInterface>(op);
    allSymUsesVisible |= !symbol || symbol.isPrivate();
  } else {
    // Otherwise if 'op' is not a symbol table, any nested symbols are
    // guaranteed to be hidden.
    allSymUsesVisible = true;
  }

  for (Region &region : op->getRegions())
    for (Block &block : region)
      for (Operation &nestedOp : block)
        walkSymbolTables(&nestedOp, allSymUsesVisible, callback);

  // If 'op' had the symbol table trait, visit it after any nested symbol
  // tables.
  if (isSymbolTable)
    callback(op, allSymUsesVisible);
}

/// Returns the operation registered with the given symbol name with the
/// regions of 'symbolTableOp'. 'symbolTableOp' is required to be an operation
/// with the 'OpTrait::SymbolTable' trait. Returns nullptr if no valid symbol
/// was found.
Operation *SymbolTable::lookupSymbolIn(Operation *symbolTableOp,
                                       StringAttr symbol) {
  assert(symbolTableOp->hasTrait<OpTrait::SymbolTable>());
  Region &region = symbolTableOp->getRegion(0);
  if (region.empty())
    return nullptr;

  // Look for a symbol with the given name.
  StringAttr symbolNameId = StringAttr::get(symbolTableOp->getContext(),
                                            SymbolTable::getSymbolAttrName());
  for (auto &op : region.front())
    if (getNameIfSymbol(&op, symbolNameId) == symbol)
      return &op;
  return nullptr;
}
Operation *SymbolTable::lookupSymbolIn(Operation *symbolTableOp,
                                       SymbolRefAttr symbol) {
  SmallVector<Operation *, 4> resolvedSymbols;
  if (failed(lookupSymbolIn(symbolTableOp, symbol, resolvedSymbols)))
    return nullptr;
  return resolvedSymbols.back();
}

/// Internal implementation of `lookupSymbolIn` that allows for specialized
/// implementations of the lookup function.
static LogicalResult lookupSymbolInImpl(
    Operation *symbolTableOp, SymbolRefAttr symbol,
    SmallVectorImpl<Operation *> &symbols,
    function_ref<Operation *(Operation *, StringAttr)> lookupSymbolFn) {
  assert(symbolTableOp->hasTrait<OpTrait::SymbolTable>());

  // Lookup the root reference for this symbol.
  symbolTableOp = lookupSymbolFn(symbolTableOp, symbol.getRootReference());
  if (!symbolTableOp)
    return failure();
  symbols.push_back(symbolTableOp);

  // If there are no nested references, just return the root symbol directly.
  ArrayRef<FlatSymbolRefAttr> nestedRefs = symbol.getNestedReferences();
  if (nestedRefs.empty())
    return success();

  // Verify that the root is also a symbol table.
  if (!symbolTableOp->hasTrait<OpTrait::SymbolTable>())
    return failure();

  // Otherwise, lookup each of the nested non-leaf references and ensure that
  // each corresponds to a valid symbol table.
  for (FlatSymbolRefAttr ref : nestedRefs.drop_back()) {
    symbolTableOp = lookupSymbolFn(symbolTableOp, ref.getAttr());
    if (!symbolTableOp || !symbolTableOp->hasTrait<OpTrait::SymbolTable>())
      return failure();
    symbols.push_back(symbolTableOp);
  }
  symbols.push_back(lookupSymbolFn(symbolTableOp, symbol.getLeafReference()));
  return success(symbols.back());
}

LogicalResult
SymbolTable::lookupSymbolIn(Operation *symbolTableOp, SymbolRefAttr symbol,
                            SmallVectorImpl<Operation *> &symbols) {
  auto lookupFn = [](Operation *symbolTableOp, StringAttr symbol) {
    return lookupSymbolIn(symbolTableOp, symbol);
  };
  return lookupSymbolInImpl(symbolTableOp, symbol, symbols, lookupFn);
}

/// Returns the operation registered with the given symbol name within the
/// closes parent operation with the 'OpTrait::SymbolTable' trait. Returns
/// nullptr if no valid symbol was found.
Operation *SymbolTable::lookupNearestSymbolFrom(Operation *from,
                                                StringAttr symbol) {
  Operation *symbolTableOp = getNearestSymbolTable(from);
  return symbolTableOp ? lookupSymbolIn(symbolTableOp, symbol) : nullptr;
}
Operation *SymbolTable::lookupNearestSymbolFrom(Operation *from,
                                                SymbolRefAttr symbol) {
  Operation *symbolTableOp = getNearestSymbolTable(from);
  return symbolTableOp ? lookupSymbolIn(symbolTableOp, symbol) : nullptr;
}

raw_ostream &mlir::operator<<(raw_ostream &os,
                              SymbolTable::Visibility visibility) {
  switch (visibility) {
  case SymbolTable::Visibility::Public:
    return os << "public";
  case SymbolTable::Visibility::Private:
    return os << "private";
  case SymbolTable::Visibility::Nested:
    return os << "nested";
  }
  llvm_unreachable("Unexpected visibility");
}

//===----------------------------------------------------------------------===//
// SymbolTable Trait Types
//===----------------------------------------------------------------------===//

LogicalResult detail::verifySymbolTable(Operation *op) {
  if (op->getNumRegions() != 1)
    return op->emitOpError()
           << "Operations with a 'SymbolTable' must have exactly one region";
  if (!op->getRegion(0).hasOneBlock())
    return op->emitOpError()
           << "Operations with a 'SymbolTable' must have exactly one block";

  // Check that all symbols are uniquely named within child regions.
  DenseMap<Attribute, Location> nameToOrigLoc;
  for (auto &block : op->getRegion(0)) {
    for (auto &op : block) {
      // Check for a symbol name attribute.
      auto nameAttr =
          op.getAttrOfType<StringAttr>(mlir::SymbolTable::getSymbolAttrName());
      if (!nameAttr)
        continue;

      // Try to insert this symbol into the table.
      auto it = nameToOrigLoc.try_emplace(nameAttr, op.getLoc());
      if (!it.second)
        return op.emitError()
            .append("redefinition of symbol named '", nameAttr.getValue(), "'")
            .attachNote(it.first->second)
            .append("see existing symbol definition here");
    }
  }

  // Verify any nested symbol user operations.
  SymbolTableCollection symbolTable;
  auto verifySymbolUserFn = [&](Operation *op) -> std::optional<WalkResult> {
    if (SymbolUserOpInterface user = dyn_cast<SymbolUserOpInterface>(op))
      return WalkResult(user.verifySymbolUses(symbolTable));
    return WalkResult::advance();
  };

  std::optional<WalkResult> result =
      walkSymbolTable(op->getRegions(), verifySymbolUserFn);
  return success(result && !result->wasInterrupted());
}

LogicalResult detail::verifySymbol(Operation *op) {
  // Verify the name attribute.
  if (!op->getAttrOfType<StringAttr>(mlir::SymbolTable::getSymbolAttrName()))
    return op->emitOpError() << "requires string attribute '"
                             << mlir::SymbolTable::getSymbolAttrName() << "'";

  // Verify the visibility attribute.
  if (Attribute vis = op->getAttr(mlir::SymbolTable::getVisibilityAttrName())) {
    StringAttr visStrAttr = llvm::dyn_cast<StringAttr>(vis);
    if (!visStrAttr)
      return op->emitOpError() << "requires visibility attribute '"
                               << mlir::SymbolTable::getVisibilityAttrName()
                               << "' to be a string attribute, but got " << vis;

    if (!llvm::is_contained(ArrayRef<StringRef>{"public", "private", "nested"},
                            visStrAttr.getValue()))
      return op->emitOpError()
             << "visibility expected to be one of [\"public\", \"private\", "
                "\"nested\"], but got "
             << visStrAttr;
  }
  return success();
}

//===----------------------------------------------------------------------===//
// Symbol Use Lists
//===----------------------------------------------------------------------===//

/// Walk all of the symbol references within the given operation, invoking the
/// provided callback for each found use. The callbacks takes the use of the
/// symbol.
static WalkResult
walkSymbolRefs(Operation *op,
               function_ref<WalkResult(SymbolTable::SymbolUse)> callback) {
  return op->getAttrDictionary().walk<WalkOrder::PreOrder>(
      [&](SymbolRefAttr symbolRef) {
        if (callback({op, symbolRef}).wasInterrupted())
          return WalkResult::interrupt();

        // Don't walk nested references.
        return WalkResult::skip();
      });
}

/// Walk all of the uses, for any symbol, that are nested within the given
/// regions, invoking the provided callback for each. This does not traverse
/// into any nested symbol tables.
static std::optional<WalkResult>
walkSymbolUses(MutableArrayRef<Region> regions,
               function_ref<WalkResult(SymbolTable::SymbolUse)> callback) {
  return walkSymbolTable(regions,
                         [&](Operation *op) -> std::optional<WalkResult> {
                           // Check that this isn't a potentially unknown symbol
                           // table.
                           if (isPotentiallyUnknownSymbolTable(op))
                             return std::nullopt;

                           return walkSymbolRefs(op, callback);
                         });
}
/// Walk all of the uses, for any symbol, that are nested within the given
/// operation 'from', invoking the provided callback for each. This does not
/// traverse into any nested symbol tables.
static std::optional<WalkResult>
walkSymbolUses(Operation *from,
               function_ref<WalkResult(SymbolTable::SymbolUse)> callback) {
  // If this operation has regions, and it, as well as its dialect, isn't
  // registered then conservatively fail. The operation may define a
  // symbol table, so we can't opaquely know if we should traverse to find
  // nested uses.
  if (isPotentiallyUnknownSymbolTable(from))
    return std::nullopt;

  // Walk the uses on this operation.
  if (walkSymbolRefs(from, callback).wasInterrupted())
    return WalkResult::interrupt();

  // Only recurse if this operation is not a symbol table. A symbol table
  // defines a new scope, so we can't walk the attributes from within the symbol
  // table op.
  if (!from->hasTrait<OpTrait::SymbolTable>())
    return walkSymbolUses(from->getRegions(), callback);
  return WalkResult::advance();
}

namespace {
/// This class represents a single symbol scope. A symbol scope represents the
/// set of operations nested within a symbol table that may reference symbols
/// within that table. A symbol scope does not contain the symbol table
/// operation itself, just its contained operations. A scope ends at leaf
/// operations or another symbol table operation.
struct SymbolScope {
  /// Walk the symbol uses within this scope, invoking the given callback.
  /// This variant is used when the callback type matches that expected by
  /// 'walkSymbolUses'.
  template <typename CallbackT,
            std::enable_if_t<!std::is_same<
                typename llvm::function_traits<CallbackT>::result_t,
                void>::value> * = nullptr>
  std::optional<WalkResult> walk(CallbackT cback) {
    if (Region *region = llvm::dyn_cast_if_present<Region *>(limit))
      return walkSymbolUses(*region, cback);
    return walkSymbolUses(cast<Operation *>(limit), cback);
  }
  /// This variant is used when the callback type matches a stripped down type:
  /// void(SymbolTable::SymbolUse use)
  template <typename CallbackT,
            std::enable_if_t<std::is_same<
                typename llvm::function_traits<CallbackT>::result_t,
                void>::value> * = nullptr>
  std::optional<WalkResult> walk(CallbackT cback) {
    return walk([=](SymbolTable::SymbolUse use) {
      return cback(use), WalkResult::advance();
    });
  }

  /// Walk all of the operations nested under the current scope without
  /// traversing into any nested symbol tables.
  template <typename CallbackT>
  std::optional<WalkResult> walkSymbolTable(CallbackT &&cback) {
    if (Region *region = llvm::dyn_cast_if_present<Region *>(limit))
      return ::walkSymbolTable(*region, cback);
    return ::walkSymbolTable(cast<Operation *>(limit), cback);
  }

  /// The representation of the symbol within this scope.
  SymbolRefAttr symbol;

  /// The IR unit representing this scope.
  llvm::PointerUnion<Operation *, Region *> limit;
};
} // namespace

/// Collect all of the symbol scopes from 'symbol' to (inclusive) 'limit'.
static SmallVector<SymbolScope, 2> collectSymbolScopes(Operation *symbol,
                                                       Operation *limit) {
  StringAttr symName = SymbolTable::getSymbolName(symbol);
  assert(!symbol->hasTrait<OpTrait::SymbolTable>() || symbol != limit);

  // Compute the ancestors of 'limit'.
  SetVector<Operation *, SmallVector<Operation *, 4>,
            SmallPtrSet<Operation *, 4>>
      limitAncestors;
  Operation *limitAncestor = limit;
  do {
    // Check to see if 'symbol' is an ancestor of 'limit'.
    if (limitAncestor == symbol) {
      // Check that the nearest symbol table is 'symbol's parent. SymbolRefAttr
      // doesn't support parent references.
      if (SymbolTable::getNearestSymbolTable(limit->getParentOp()) ==
          symbol->getParentOp())
        return {{SymbolRefAttr::get(symName), limit}};
      return {};
    }

    limitAncestors.insert(limitAncestor);
  } while ((limitAncestor = limitAncestor->getParentOp()));

  // Try to find the first ancestor of 'symbol' that is an ancestor of 'limit'.
  Operation *commonAncestor = symbol->getParentOp();
  do {
    if (limitAncestors.count(commonAncestor))
      break;
  } while ((commonAncestor = commonAncestor->getParentOp()));
  assert(commonAncestor && "'limit' and 'symbol' have no common ancestor");

  // Compute the set of valid nested references for 'symbol' as far up to the
  // common ancestor as possible.
  SmallVector<SymbolRefAttr, 2> references;
  bool collectedAllReferences = succeeded(
      collectValidReferencesFor(symbol, symName, commonAncestor, references));

  // Handle the case where the common ancestor is 'limit'.
  if (commonAncestor == limit) {
    SmallVector<SymbolScope, 2> scopes;

    // Walk each of the ancestors of 'symbol', calling the compute function for
    // each one.
    Operation *limitIt = symbol->getParentOp();
    for (size_t i = 0, e = references.size(); i != e;
         ++i, limitIt = limitIt->getParentOp()) {
      assert(limitIt->hasTrait<OpTrait::SymbolTable>());
      scopes.push_back({references[i], &limitIt->getRegion(0)});
    }
    return scopes;
  }

  // Otherwise, we just need the symbol reference for 'symbol' that will be
  // used within 'limit'. This is the last reference in the list we computed
  // above if we were able to collect all references.
  if (!collectedAllReferences)
    return {};
  return {{references.back(), limit}};
}
static SmallVector<SymbolScope, 2> collectSymbolScopes(Operation *symbol,
                                                       Region *limit) {
  auto scopes = collectSymbolScopes(symbol, limit->getParentOp());

  // If we collected some scopes to walk, make sure to constrain the one for
  // limit to the specific region requested.
  if (!scopes.empty())
    scopes.back().limit = limit;
  return scopes;
}
static SmallVector<SymbolScope, 1> collectSymbolScopes(StringAttr symbol,
                                                       Region *limit) {
  return {{SymbolRefAttr::get(symbol), limit}};
}

static SmallVector<SymbolScope, 1> collectSymbolScopes(StringAttr symbol,
                                                       Operation *limit) {
  SmallVector<SymbolScope, 1> scopes;
  auto symbolRef = SymbolRefAttr::get(symbol);
  for (auto &region : limit->getRegions())
    scopes.push_back({symbolRef, &region});
  return scopes;
}

/// Returns true if the given reference 'SubRef' is a sub reference of the
/// reference 'ref', i.e. 'ref' is a further qualified reference.
static bool isReferencePrefixOf(SymbolRefAttr subRef, SymbolRefAttr ref) {
  if (ref == subRef)
    return true;

  // If the references are not pointer equal, check to see if `subRef` is a
  // prefix of `ref`.
  if (llvm::isa<FlatSymbolRefAttr>(ref) ||
      ref.getRootReference() != subRef.getRootReference())
    return false;

  auto refLeafs = ref.getNestedReferences();
  auto subRefLeafs = subRef.getNestedReferences();
  return subRefLeafs.size() < refLeafs.size() &&
         subRefLeafs == refLeafs.take_front(subRefLeafs.size());
}

//===----------------------------------------------------------------------===//
// SymbolTable::getSymbolUses
//===----------------------------------------------------------------------===//

/// The implementation of SymbolTable::getSymbolUses below.
template <typename FromT>
static std::optional<SymbolTable::UseRange> getSymbolUsesImpl(FromT from) {
  std::vector<SymbolTable::SymbolUse> uses;
  auto walkFn = [&](SymbolTable::SymbolUse symbolUse) {
    uses.push_back(symbolUse);
    return WalkResult::advance();
  };
  auto result = walkSymbolUses(from, walkFn);
  return result ? std::optional<SymbolTable::UseRange>(std::move(uses))
                : std::nullopt;
}

/// Get an iterator range for all of the uses, for any symbol, that are nested
/// within the given operation 'from'. This does not traverse into any nested
/// symbol tables, and will also only return uses on 'from' if it does not
/// also define a symbol table. This is because we treat the region as the
/// boundary of the symbol table, and not the op itself. This function returns
/// std::nullopt if there are any unknown operations that may potentially be
/// symbol tables.
auto SymbolTable::getSymbolUses(Operation *from) -> std::optional<UseRange> {
  return getSymbolUsesImpl(from);
}
auto SymbolTable::getSymbolUses(Region *from) -> std::optional<UseRange> {
  return getSymbolUsesImpl(MutableArrayRef<Region>(*from));
}

//===----------------------------------------------------------------------===//
// SymbolTable::getSymbolUses
//===----------------------------------------------------------------------===//

/// The implementation of SymbolTable::getSymbolUses below.
template <typename SymbolT, typename IRUnitT>
static std::optional<SymbolTable::UseRange> getSymbolUsesImpl(SymbolT symbol,
                                                              IRUnitT *limit) {
  std::vector<SymbolTable::SymbolUse> uses;
  for (SymbolScope &scope : collectSymbolScopes(symbol, limit)) {
    if (!scope.walk([&](SymbolTable::SymbolUse symbolUse) {
          if (isReferencePrefixOf(scope.symbol, symbolUse.getSymbolRef()))
            uses.push_back(symbolUse);
        }))
      return std::nullopt;
  }
  return SymbolTable::UseRange(std::move(uses));
}

/// Get all of the uses of the given symbol that are nested within the given
/// operation 'from'. This does not traverse into any nested symbol tables.
/// This function returns std::nullopt if there are any unknown operations that
/// may potentially be symbol tables.
auto SymbolTable::getSymbolUses(StringAttr symbol, Operation *from)
    -> std::optional<UseRange> {
  return getSymbolUsesImpl(symbol, from);
}
auto SymbolTable::getSymbolUses(Operation *symbol, Operation *from)
    -> std::optional<UseRange> {
  return getSymbolUsesImpl(symbol, from);
}
auto SymbolTable::getSymbolUses(StringAttr symbol, Region *from)
    -> std::optional<UseRange> {
  return getSymbolUsesImpl(symbol, from);
}
auto SymbolTable::getSymbolUses(Operation *symbol, Region *from)
    -> std::optional<UseRange> {
  return getSymbolUsesImpl(symbol, from);
}

//===----------------------------------------------------------------------===//
// SymbolTable::symbolKnownUseEmpty
//===----------------------------------------------------------------------===//

/// The implementation of SymbolTable::symbolKnownUseEmpty below.
template <typename SymbolT, typename IRUnitT>
static bool symbolKnownUseEmptyImpl(SymbolT symbol, IRUnitT *limit) {
  for (SymbolScope &scope : collectSymbolScopes(symbol, limit)) {
    // Walk all of the symbol uses looking for a reference to 'symbol'.
    if (scope.walk([&](SymbolTable::SymbolUse symbolUse) {
          return isReferencePrefixOf(scope.symbol, symbolUse.getSymbolRef())
                     ? WalkResult::interrupt()
                     : WalkResult::advance();
        }) != WalkResult::advance())
      return false;
  }
  return true;
}

/// Return if the given symbol is known to have no uses that are nested within
/// the given operation 'from'. This does not traverse into any nested symbol
/// tables. This function will also return false if there are any unknown
/// operations that may potentially be symbol tables.
bool SymbolTable::symbolKnownUseEmpty(StringAttr symbol, Operation *from) {
  return symbolKnownUseEmptyImpl(symbol, from);
}
bool SymbolTable::symbolKnownUseEmpty(Operation *symbol, Operation *from) {
  return symbolKnownUseEmptyImpl(symbol, from);
}
bool SymbolTable::symbolKnownUseEmpty(StringAttr symbol, Region *from) {
  return symbolKnownUseEmptyImpl(symbol, from);
}
bool SymbolTable::symbolKnownUseEmpty(Operation *symbol, Region *from) {
  return symbolKnownUseEmptyImpl(symbol, from);
}

//===----------------------------------------------------------------------===//
// SymbolTable::replaceAllSymbolUses
//===----------------------------------------------------------------------===//

/// Generates a new symbol reference attribute with a new leaf reference.
static SymbolRefAttr generateNewRefAttr(SymbolRefAttr oldAttr,
                                        FlatSymbolRefAttr newLeafAttr) {
  if (llvm::isa<FlatSymbolRefAttr>(oldAttr))
    return newLeafAttr;
  auto nestedRefs = llvm::to_vector<2>(oldAttr.getNestedReferences());
  nestedRefs.back() = newLeafAttr;
  return SymbolRefAttr::get(oldAttr.getRootReference(), nestedRefs);
}

/// The implementation of SymbolTable::replaceAllSymbolUses below.
template <typename SymbolT, typename IRUnitT>
static LogicalResult
replaceAllSymbolUsesImpl(SymbolT symbol, StringAttr newSymbol, IRUnitT *limit) {
  // Generate a new attribute to replace the given attribute.
  FlatSymbolRefAttr newLeafAttr = FlatSymbolRefAttr::get(newSymbol);
  for (SymbolScope &scope : collectSymbolScopes(symbol, limit)) {
    SymbolRefAttr oldAttr = scope.symbol;
    SymbolRefAttr newAttr = generateNewRefAttr(scope.symbol, newLeafAttr);
    AttrTypeReplacer replacer;
    replacer.addReplacement(
        [&](SymbolRefAttr attr) -> std::pair<Attribute, WalkResult> {
          // Regardless of the match, don't walk nested SymbolRefAttrs, we don't
          // want to accidentally replace an inner reference.
          if (attr == oldAttr)
            return {newAttr, WalkResult::skip()};
          // Handle prefix matches.
          if (isReferencePrefixOf(oldAttr, attr)) {
            auto oldNestedRefs = oldAttr.getNestedReferences();
            auto nestedRefs = attr.getNestedReferences();
            if (oldNestedRefs.empty())
              return {SymbolRefAttr::get(newSymbol, nestedRefs),
                      WalkResult::skip()};

            auto newNestedRefs = llvm::to_vector<4>(nestedRefs);
            newNestedRefs[oldNestedRefs.size() - 1] = newLeafAttr;
            return {SymbolRefAttr::get(attr.getRootReference(), newNestedRefs),
                    WalkResult::skip()};
          }
          return {attr, WalkResult::skip()};
        });

    auto walkFn = [&](Operation *op) -> std::optional<WalkResult> {
      replacer.replaceElementsIn(op);
      return WalkResult::advance();
    };
    if (!scope.walkSymbolTable(walkFn))
      return failure();
  }
  return success();
}

/// Attempt to replace all uses of the given symbol 'oldSymbol' with the
/// provided symbol 'newSymbol' that are nested within the given operation
/// 'from'. This does not traverse into any nested symbol tables. If there are
/// any unknown operations that may potentially be symbol tables, no uses are
/// replaced and failure is returned.
LogicalResult SymbolTable::replaceAllSymbolUses(StringAttr oldSymbol,
                                                StringAttr newSymbol,
                                                Operation *from) {
  return replaceAllSymbolUsesImpl(oldSymbol, newSymbol, from);
}
LogicalResult SymbolTable::replaceAllSymbolUses(Operation *oldSymbol,
                                                StringAttr newSymbol,
                                                Operation *from) {
  return replaceAllSymbolUsesImpl(oldSymbol, newSymbol, from);
}
LogicalResult SymbolTable::replaceAllSymbolUses(StringAttr oldSymbol,
                                                StringAttr newSymbol,
                                                Region *from) {
  return replaceAllSymbolUsesImpl(oldSymbol, newSymbol, from);
}
LogicalResult SymbolTable::replaceAllSymbolUses(Operation *oldSymbol,
                                                StringAttr newSymbol,
                                                Region *from) {
  return replaceAllSymbolUsesImpl(oldSymbol, newSymbol, from);
}

//===----------------------------------------------------------------------===//
// SymbolTableCollection
//===----------------------------------------------------------------------===//

Operation *SymbolTableCollection::lookupSymbolIn(Operation *symbolTableOp,
                                                 StringAttr symbol) {
  return getSymbolTable(symbolTableOp).lookup(symbol);
}
Operation *SymbolTableCollection::lookupSymbolIn(Operation *symbolTableOp,
                                                 SymbolRefAttr name) {
  SmallVector<Operation *, 4> symbols;
  if (failed(lookupSymbolIn(symbolTableOp, name, symbols)))
    return nullptr;
  return symbols.back();
}
/// A variant of 'lookupSymbolIn' that returns all of the symbols referenced by
/// a given SymbolRefAttr. Returns failure if any of the nested references could
/// not be resolved.
LogicalResult
SymbolTableCollection::lookupSymbolIn(Operation *symbolTableOp,
                                      SymbolRefAttr name,
                                      SmallVectorImpl<Operation *> &symbols) {
  auto lookupFn = [this](Operation *symbolTableOp, StringAttr symbol) {
    return lookupSymbolIn(symbolTableOp, symbol);
  };
  return lookupSymbolInImpl(symbolTableOp, name, symbols, lookupFn);
}

/// Returns the operation registered with the given symbol name within the
/// closest parent operation of, or including, 'from' with the
/// 'OpTrait::SymbolTable' trait. Returns nullptr if no valid symbol was
/// found.
Operation *SymbolTableCollection::lookupNearestSymbolFrom(Operation *from,
                                                          StringAttr symbol) {
  Operation *symbolTableOp = SymbolTable::getNearestSymbolTable(from);
  return symbolTableOp ? lookupSymbolIn(symbolTableOp, symbol) : nullptr;
}
Operation *
SymbolTableCollection::lookupNearestSymbolFrom(Operation *from,
                                               SymbolRefAttr symbol) {
  Operation *symbolTableOp = SymbolTable::getNearestSymbolTable(from);
  return symbolTableOp ? lookupSymbolIn(symbolTableOp, symbol) : nullptr;
}

/// Lookup, or create, a symbol table for an operation.
SymbolTable &SymbolTableCollection::getSymbolTable(Operation *op) {
  auto it = symbolTables.try_emplace(op, nullptr);
  if (it.second)
    it.first->second = std::make_unique<SymbolTable>(op);
  return *it.first->second;
}

void SymbolTableCollection::invalidateSymbolTable(Operation *op) {
  symbolTables.erase(op);
}

//===----------------------------------------------------------------------===//
// LockedSymbolTableCollection
//===----------------------------------------------------------------------===//

Operation *LockedSymbolTableCollection::lookupSymbolIn(Operation *symbolTableOp,
                                                       StringAttr symbol) {
  return getSymbolTable(symbolTableOp).lookup(symbol);
}

Operation *
LockedSymbolTableCollection::lookupSymbolIn(Operation *symbolTableOp,
                                            FlatSymbolRefAttr symbol) {
  return lookupSymbolIn(symbolTableOp, symbol.getAttr());
}

Operation *LockedSymbolTableCollection::lookupSymbolIn(Operation *symbolTableOp,
                                                       SymbolRefAttr name) {
  SmallVector<Operation *> symbols;
  if (failed(lookupSymbolIn(symbolTableOp, name, symbols)))
    return nullptr;
  return symbols.back();
}

LogicalResult LockedSymbolTableCollection::lookupSymbolIn(
    Operation *symbolTableOp, SymbolRefAttr name,
    SmallVectorImpl<Operation *> &symbols) {
  auto lookupFn = [this](Operation *symbolTableOp, StringAttr symbol) {
    return lookupSymbolIn(symbolTableOp, symbol);
  };
  return lookupSymbolInImpl(symbolTableOp, name, symbols, lookupFn);
}

SymbolTable &
LockedSymbolTableCollection::getSymbolTable(Operation *symbolTableOp) {
  assert(symbolTableOp->hasTrait<OpTrait::SymbolTable>());
  // Try to find an existing symbol table.
  {
    llvm::sys::SmartScopedReader<true> lock(mutex);
    auto it = collection.symbolTables.find(symbolTableOp);
    if (it != collection.symbolTables.end())
      return *it->second;
  }
  // Create a symbol table for the operation. Perform construction outside of
  // the critical section.
  auto symbolTable = std::make_unique<SymbolTable>(symbolTableOp);
  // Insert the constructed symbol table.
  llvm::sys::SmartScopedWriter<true> lock(mutex);
  return *collection.symbolTables
              .insert({symbolTableOp, std::move(symbolTable)})
              .first->second;
}

//===----------------------------------------------------------------------===//
// SymbolUserMap
//===----------------------------------------------------------------------===//

SymbolUserMap::SymbolUserMap(SymbolTableCollection &symbolTable,
                             Operation *symbolTableOp)
    : symbolTable(symbolTable) {
  // Walk each of the symbol tables looking for discardable callgraph nodes.
  SmallVector<Operation *> symbols;
  auto walkFn = [&](Operation *symbolTableOp, bool allUsesVisible) {
    for (Operation &nestedOp : symbolTableOp->getRegion(0).getOps()) {
      auto symbolUses = SymbolTable::getSymbolUses(&nestedOp);
      assert(symbolUses && "expected uses to be valid");

      for (const SymbolTable::SymbolUse &use : *symbolUses) {
        symbols.clear();
        (void)symbolTable.lookupSymbolIn(symbolTableOp, use.getSymbolRef(),
                                         symbols);
        for (Operation *symbolOp : symbols)
          symbolToUsers[symbolOp].insert(use.getUser());
      }
    }
  };
  // We just set `allSymUsesVisible` to false here because it isn't necessary
  // for building the user map.
  SymbolTable::walkSymbolTables(symbolTableOp, /*allSymUsesVisible=*/false,
                                walkFn);
}

void SymbolUserMap::replaceAllUsesWith(Operation *symbol,
                                       StringAttr newSymbolName) {
  auto it = symbolToUsers.find(symbol);
  if (it == symbolToUsers.end())
    return;

  // Replace the uses within the users of `symbol`.
  for (Operation *user : it->second)
    (void)SymbolTable::replaceAllSymbolUses(symbol, newSymbolName, user);

  // Move the current users of `symbol` to the new symbol if it is in the
  // symbol table.
  Operation *newSymbol =
      symbolTable.lookupSymbolIn(symbol->getParentOp(), newSymbolName);
  if (newSymbol != symbol) {
    // Transfer over the users to the new symbol.  The reference to the old one
    // is fetched again as the iterator is invalidated during the insertion.
    auto newIt = symbolToUsers.try_emplace(newSymbol);
    auto oldIt = symbolToUsers.find(symbol);
    assert(oldIt != symbolToUsers.end() && "missing old users list");
    if (newIt.second)
      newIt.first->second = std::move(oldIt->second);
    else
      newIt.first->second.set_union(oldIt->second);
    symbolToUsers.erase(oldIt);
  }
}

//===----------------------------------------------------------------------===//
// Visibility parsing implementation.
//===----------------------------------------------------------------------===//

ParseResult impl::parseOptionalVisibilityKeyword(OpAsmParser &parser,
                                                 NamedAttrList &attrs) {
  StringRef visibility;
  if (parser.parseOptionalKeyword(&visibility, {"public", "private", "nested"}))
    return failure();

  StringAttr visibilityAttr = parser.getBuilder().getStringAttr(visibility);
  attrs.push_back(parser.getBuilder().getNamedAttr(
      SymbolTable::getVisibilityAttrName(), visibilityAttr));
  return success();
}

//===----------------------------------------------------------------------===//
// Symbol Interfaces
//===----------------------------------------------------------------------===//

/// Include the generated symbol interfaces.
#include "mlir/IR/SymbolInterfaces.cpp.inc"
