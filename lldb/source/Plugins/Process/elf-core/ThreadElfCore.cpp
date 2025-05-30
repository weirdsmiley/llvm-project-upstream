//===-- ThreadElfCore.cpp -------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/StopInfo.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/UnixSignals.h"
#include "lldb/Target/Unwind.h"
#include "lldb/Utility/DataExtractor.h"
#include "lldb/Utility/LLDBLog.h"
#include "lldb/Utility/Log.h"
#include "lldb/Utility/ProcessInfo.h"

#include "Plugins/Process/Utility/RegisterContextFreeBSD_i386.h"
#include "Plugins/Process/Utility/RegisterContextFreeBSD_mips64.h"
#include "Plugins/Process/Utility/RegisterContextFreeBSD_powerpc.h"
#include "Plugins/Process/Utility/RegisterContextFreeBSD_x86_64.h"
#include "Plugins/Process/Utility/RegisterContextLinux_i386.h"
#include "Plugins/Process/Utility/RegisterContextLinux_s390x.h"
#include "Plugins/Process/Utility/RegisterContextLinux_x86_64.h"
#include "Plugins/Process/Utility/RegisterContextNetBSD_i386.h"
#include "Plugins/Process/Utility/RegisterContextNetBSD_x86_64.h"
#include "Plugins/Process/Utility/RegisterContextOpenBSD_i386.h"
#include "Plugins/Process/Utility/RegisterContextOpenBSD_x86_64.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_arm.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_arm64.h"
#include "Plugins/Process/Utility/RegisterInfoPOSIX_ppc64le.h"
#include "ProcessElfCore.h"
#include "RegisterContextLinuxCore_x86_64.h"
#include "RegisterContextPOSIXCore_arm.h"
#include "RegisterContextPOSIXCore_arm64.h"
#include "RegisterContextPOSIXCore_loongarch64.h"
#include "RegisterContextPOSIXCore_mips64.h"
#include "RegisterContextPOSIXCore_powerpc.h"
#include "RegisterContextPOSIXCore_ppc64le.h"
#include "RegisterContextPOSIXCore_riscv32.h"
#include "RegisterContextPOSIXCore_riscv64.h"
#include "RegisterContextPOSIXCore_s390x.h"
#include "RegisterContextPOSIXCore_x86_64.h"
#include "ThreadElfCore.h"

#include <memory>

using namespace lldb;
using namespace lldb_private;

// Construct a Thread object with given data
ThreadElfCore::ThreadElfCore(Process &process, const ThreadData &td)
    : Thread(process, td.tid), m_thread_name(td.name), m_thread_reg_ctx_sp(),
      m_gpregset_data(td.gpregset), m_notes(td.notes),
      m_siginfo_bytes(std::move(td.siginfo_bytes)), m_signo(td.signo) {}

ThreadElfCore::~ThreadElfCore() { DestroyThread(); }

void ThreadElfCore::RefreshStateAfterStop() {
  GetRegisterContext()->InvalidateIfNeeded(false);
}

RegisterContextSP ThreadElfCore::GetRegisterContext() {
  if (!m_reg_context_sp) {
    m_reg_context_sp = CreateRegisterContextForFrame(nullptr);
  }
  return m_reg_context_sp;
}

RegisterContextSP
ThreadElfCore::CreateRegisterContextForFrame(StackFrame *frame) {
  RegisterContextSP reg_ctx_sp;
  uint32_t concrete_frame_idx = 0;
  Log *log = GetLog(LLDBLog::Thread);

  if (frame)
    concrete_frame_idx = frame->GetConcreteFrameIndex();

  bool is_linux = false;
  if (concrete_frame_idx == 0) {
    if (m_thread_reg_ctx_sp)
      return m_thread_reg_ctx_sp;

    ProcessElfCore *process = static_cast<ProcessElfCore *>(GetProcess().get());
    ArchSpec arch = process->GetArchitecture();
    RegisterInfoInterface *reg_interface = nullptr;

    switch (arch.GetTriple().getOS()) {
    case llvm::Triple::FreeBSD: {
      switch (arch.GetMachine()) {
      case llvm::Triple::aarch64:
      case llvm::Triple::arm:
        break;
      case llvm::Triple::ppc:
        reg_interface = new RegisterContextFreeBSD_powerpc32(arch);
        break;
      case llvm::Triple::ppc64:
      case llvm::Triple::ppc64le:
        reg_interface = new RegisterContextFreeBSD_powerpc64(arch);
        break;
      case llvm::Triple::mips64:
        reg_interface = new RegisterContextFreeBSD_mips64(arch);
        break;
      case llvm::Triple::x86:
        reg_interface = new RegisterContextFreeBSD_i386(arch);
        break;
      case llvm::Triple::x86_64:
        reg_interface = new RegisterContextFreeBSD_x86_64(arch);
        break;
      default:
        break;
      }
      break;
    }

    case llvm::Triple::NetBSD: {
      switch (arch.GetMachine()) {
      case llvm::Triple::aarch64:
        break;
      case llvm::Triple::x86:
        reg_interface = new RegisterContextNetBSD_i386(arch);
        break;
      case llvm::Triple::x86_64:
        reg_interface = new RegisterContextNetBSD_x86_64(arch);
        break;
      default:
        break;
      }
      break;
    }

    case llvm::Triple::Linux: {
      is_linux = true;
      switch (arch.GetMachine()) {
      case llvm::Triple::aarch64:
        break;
      case llvm::Triple::ppc64le:
        reg_interface = new RegisterInfoPOSIX_ppc64le(arch);
        break;
      case llvm::Triple::systemz:
        reg_interface = new RegisterContextLinux_s390x(arch);
        break;
      case llvm::Triple::x86:
        reg_interface = new RegisterContextLinux_i386(arch);
        break;
      case llvm::Triple::x86_64:
        reg_interface = new RegisterContextLinux_x86_64(arch);
        break;
      default:
        break;
      }
      break;
    }

    case llvm::Triple::OpenBSD: {
      switch (arch.GetMachine()) {
      case llvm::Triple::aarch64:
        break;
      case llvm::Triple::x86:
        reg_interface = new RegisterContextOpenBSD_i386(arch);
        break;
      case llvm::Triple::x86_64:
        reg_interface = new RegisterContextOpenBSD_x86_64(arch);
        break;
      default:
        break;
      }
      break;
    }

    default:
      break;
    }

    if (!reg_interface && arch.GetMachine() != llvm::Triple::aarch64 &&
        arch.GetMachine() != llvm::Triple::arm &&
        arch.GetMachine() != llvm::Triple::loongarch64 &&
        arch.GetMachine() != llvm::Triple::riscv64 &&
        arch.GetMachine() != llvm::Triple::riscv32) {
      LLDB_LOGF(log, "elf-core::%s:: Architecture(%d) or OS(%d) not supported",
                __FUNCTION__, arch.GetMachine(), arch.GetTriple().getOS());
      assert(false && "Architecture or OS not supported");
    }

    switch (arch.GetMachine()) {
    case llvm::Triple::aarch64:
      m_thread_reg_ctx_sp = RegisterContextCorePOSIX_arm64::Create(
          *this, arch, m_gpregset_data, m_notes);
      break;
    case llvm::Triple::arm:
      m_thread_reg_ctx_sp = std::make_shared<RegisterContextCorePOSIX_arm>(
          *this, std::make_unique<RegisterInfoPOSIX_arm>(arch), m_gpregset_data,
          m_notes);
      break;
    case llvm::Triple::loongarch64:
      m_thread_reg_ctx_sp = RegisterContextCorePOSIX_loongarch64::Create(
          *this, arch, m_gpregset_data, m_notes);
      break;
    case llvm::Triple::riscv32:
      m_thread_reg_ctx_sp = RegisterContextCorePOSIX_riscv32::Create(
          *this, arch, m_gpregset_data, m_notes);
      break;
    case llvm::Triple::riscv64:
      m_thread_reg_ctx_sp = RegisterContextCorePOSIX_riscv64::Create(
          *this, arch, m_gpregset_data, m_notes);
      break;
    case llvm::Triple::mipsel:
    case llvm::Triple::mips:
      m_thread_reg_ctx_sp = std::make_shared<RegisterContextCorePOSIX_mips64>(
          *this, reg_interface, m_gpregset_data, m_notes);
      break;
    case llvm::Triple::mips64:
    case llvm::Triple::mips64el:
      m_thread_reg_ctx_sp = std::make_shared<RegisterContextCorePOSIX_mips64>(
          *this, reg_interface, m_gpregset_data, m_notes);
      break;
    case llvm::Triple::ppc:
    case llvm::Triple::ppc64:
      m_thread_reg_ctx_sp = std::make_shared<RegisterContextCorePOSIX_powerpc>(
          *this, reg_interface, m_gpregset_data, m_notes);
      break;
    case llvm::Triple::ppc64le:
      m_thread_reg_ctx_sp = std::make_shared<RegisterContextCorePOSIX_ppc64le>(
          *this, reg_interface, m_gpregset_data, m_notes);
      break;
    case llvm::Triple::systemz:
      m_thread_reg_ctx_sp = std::make_shared<RegisterContextCorePOSIX_s390x>(
          *this, reg_interface, m_gpregset_data, m_notes);
      break;
    case llvm::Triple::x86:
    case llvm::Triple::x86_64:
      if (is_linux) {
        m_thread_reg_ctx_sp = std::make_shared<RegisterContextLinuxCore_x86_64>(
              *this, reg_interface, m_gpregset_data, m_notes);
      } else {
        m_thread_reg_ctx_sp = std::make_shared<RegisterContextCorePOSIX_x86_64>(
              *this, reg_interface, m_gpregset_data, m_notes);
      }
      break;
    default:
      break;
    }

    reg_ctx_sp = m_thread_reg_ctx_sp;
  } else {
    reg_ctx_sp = GetUnwinder().CreateRegisterContextForFrame(frame);
  }
  return reg_ctx_sp;
}

llvm::Expected<std::unique_ptr<llvm::MemoryBuffer>>
ThreadElfCore::GetSiginfo(size_t max_size) const {
  if (m_siginfo_bytes.empty())
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "no siginfo note");

  return llvm::MemoryBuffer::getMemBufferCopy(m_siginfo_bytes,
                                              "siginfo note bytes");
}

bool ThreadElfCore::CalculateStopInfo() {
  ProcessSP process_sp(GetProcess());
  if (!process_sp)
    return false;

  PlatformSP platform_sp = process_sp->GetTarget().GetPlatform();
  if (platform_sp) {
    lldb::StopInfoSP stopinfo_sp = platform_sp->GetStopInfoFromSiginfo(*this);
    // The platform SP can optionally handle creating the stop info from the
    // siginfo value however it's not guaraunteed to be implemented on every
    // platform, so if we fall through this case, we create from just the signo.
    if (stopinfo_sp) {
      SetStopInfo(std::move(stopinfo_sp));
      return true;
    }
  }

  SetStopInfo(StopInfo::CreateStopReasonWithSignal(*this, m_signo));
  return true;
}

// Parse PRSTATUS from NOTE entry
ELFLinuxPrStatus::ELFLinuxPrStatus() {
  memset(this, 0, sizeof(ELFLinuxPrStatus));
}

size_t ELFLinuxPrStatus::GetSize(const lldb_private::ArchSpec &arch) {
  constexpr size_t mips_linux_pr_status_size_o32 = 96;
  constexpr size_t mips_linux_pr_status_size_n32 = 72;
  constexpr size_t num_ptr_size_members = 10;
  if (arch.IsMIPS()) {
    std::string abi = arch.GetTargetABI();
    assert(!abi.empty() && "ABI is not set");
    if (abi == "n64")
      return sizeof(ELFLinuxPrStatus);
    else if (abi == "o32")
      return mips_linux_pr_status_size_o32;
    // N32 ABI
    return mips_linux_pr_status_size_n32;
  }
  switch (arch.GetCore()) {
  case lldb_private::ArchSpec::eCore_x86_32_i386:
  case lldb_private::ArchSpec::eCore_x86_32_i486:
    return 72;
  default:
    if (arch.GetAddressByteSize() == 8)
      return sizeof(ELFLinuxPrStatus);
    else
      return sizeof(ELFLinuxPrStatus) - num_ptr_size_members * 4;
  }
}

Status ELFLinuxPrStatus::Parse(const DataExtractor &data,
                               const ArchSpec &arch) {
  Status error;
  if (GetSize(arch) > data.GetByteSize()) {
    error = Status::FromErrorStringWithFormat(
        "NT_PRSTATUS size should be %zu, but the remaining bytes are: %" PRIu64,
        GetSize(arch), data.GetByteSize());
    return error;
  }

  // Read field by field to correctly account for endianess of both the core
  // dump and the platform running lldb.
  offset_t offset = 0;
  si_signo = data.GetU32(&offset);
  si_code = data.GetU32(&offset);
  si_errno = data.GetU32(&offset);

  pr_cursig = data.GetU16(&offset);
  offset += 2; // pad

  pr_sigpend = data.GetAddress(&offset);
  pr_sighold = data.GetAddress(&offset);

  pr_pid = data.GetU32(&offset);
  pr_ppid = data.GetU32(&offset);
  pr_pgrp = data.GetU32(&offset);
  pr_sid = data.GetU32(&offset);

  pr_utime.tv_sec = data.GetAddress(&offset);
  pr_utime.tv_usec = data.GetAddress(&offset);

  pr_stime.tv_sec = data.GetAddress(&offset);
  pr_stime.tv_usec = data.GetAddress(&offset);

  pr_cutime.tv_sec = data.GetAddress(&offset);
  pr_cutime.tv_usec = data.GetAddress(&offset);

  pr_cstime.tv_sec = data.GetAddress(&offset);
  pr_cstime.tv_usec = data.GetAddress(&offset);

  return error;
}

static struct compat_timeval
copy_timespecs(const ProcessInstanceInfo::timespec &oth) {
  using sec_t = decltype(compat_timeval::tv_sec);
  using usec_t = decltype(compat_timeval::tv_usec);
  return {static_cast<sec_t>(oth.tv_sec), static_cast<usec_t>(oth.tv_usec)};
}

std::optional<ELFLinuxPrStatus>
ELFLinuxPrStatus::Populate(const lldb::ThreadSP &thread_sp) {
  ELFLinuxPrStatus prstatus{};
  prstatus.pr_pid = thread_sp->GetID();
  lldb::ProcessSP process_sp = thread_sp->GetProcess();
  ProcessInstanceInfo info;
  if (!process_sp->GetProcessInfo(info))
    return std::nullopt;

  prstatus.pr_ppid = info.GetParentProcessID();
  prstatus.pr_pgrp = info.GetProcessGroupID();
  prstatus.pr_sid = info.GetProcessSessionID();
  prstatus.pr_utime = copy_timespecs(info.GetUserTime());
  prstatus.pr_stime = copy_timespecs(info.GetSystemTime());
  prstatus.pr_cutime = copy_timespecs(info.GetCumulativeUserTime());
  prstatus.pr_cstime = copy_timespecs(info.GetCumulativeSystemTime());
  return prstatus;
}

// Parse PRPSINFO from NOTE entry
ELFLinuxPrPsInfo::ELFLinuxPrPsInfo() {
  memset(this, 0, sizeof(ELFLinuxPrPsInfo));
}

size_t ELFLinuxPrPsInfo::GetSize(const lldb_private::ArchSpec &arch) {
  constexpr size_t mips_linux_pr_psinfo_size_o32_n32 = 128;
  if (arch.IsMIPS()) {
    uint8_t address_byte_size = arch.GetAddressByteSize();
    if (address_byte_size == 8)
      return sizeof(ELFLinuxPrPsInfo);
    return mips_linux_pr_psinfo_size_o32_n32;
  }

  switch (arch.GetCore()) {
  case lldb_private::ArchSpec::eCore_s390x_generic:
  case lldb_private::ArchSpec::eCore_x86_64_x86_64:
    return sizeof(ELFLinuxPrPsInfo);
  case lldb_private::ArchSpec::eCore_x86_32_i386:
  case lldb_private::ArchSpec::eCore_x86_32_i486:
    return 124;
  default:
    return 0;
  }
}

Status ELFLinuxPrPsInfo::Parse(const DataExtractor &data,
                               const ArchSpec &arch) {
  Status error;
  ByteOrder byteorder = data.GetByteOrder();
  if (GetSize(arch) > data.GetByteSize()) {
    error = Status::FromErrorStringWithFormat(
        "NT_PRPSINFO size should be %zu, but the remaining bytes are: %" PRIu64,
        GetSize(arch), data.GetByteSize());
    return error;
  }
  size_t size = 0;
  offset_t offset = 0;

  pr_state = data.GetU8(&offset);
  pr_sname = data.GetU8(&offset);
  pr_zomb = data.GetU8(&offset);
  pr_nice = data.GetU8(&offset);
  if (data.GetAddressByteSize() == 8) {
    // Word align the next field on 64 bit.
    offset += 4;
  }

  pr_flag = data.GetAddress(&offset);

  if (arch.IsMIPS()) {
    // The pr_uid and pr_gid is always 32 bit irrespective of platforms
    pr_uid = data.GetU32(&offset);
    pr_gid = data.GetU32(&offset);
  } else {
    // 16 bit on 32 bit platforms, 32 bit on 64 bit platforms
    pr_uid = data.GetMaxU64(&offset, data.GetAddressByteSize() >> 1);
    pr_gid = data.GetMaxU64(&offset, data.GetAddressByteSize() >> 1);
  }

  pr_pid = data.GetU32(&offset);
  pr_ppid = data.GetU32(&offset);
  pr_pgrp = data.GetU32(&offset);
  pr_sid = data.GetU32(&offset);

  size = 16;
  data.ExtractBytes(offset, size, byteorder, pr_fname);
  offset += size;

  size = 80;
  data.ExtractBytes(offset, size, byteorder, pr_psargs);
  offset += size;

  return error;
}

std::optional<ELFLinuxPrPsInfo>
ELFLinuxPrPsInfo::Populate(const lldb::ProcessSP &process_sp) {
  ProcessInstanceInfo info;
  if (!process_sp->GetProcessInfo(info))
    return std::nullopt;

  return Populate(info, process_sp->GetState());
}

std::optional<ELFLinuxPrPsInfo>
ELFLinuxPrPsInfo::Populate(const lldb_private::ProcessInstanceInfo &info,
                           lldb::StateType process_state) {
  ELFLinuxPrPsInfo prpsinfo{};
  prpsinfo.pr_pid = info.GetProcessID();
  prpsinfo.pr_nice = info.GetPriorityValue().value_or(0);
  prpsinfo.pr_zomb = 0;
  if (auto zombie_opt = info.IsZombie(); zombie_opt.value_or(false)) {
    prpsinfo.pr_zomb = 1;
  }
  /**
   * In the linux kernel this comes from:
   * state = READ_ONCE(p->__state);
   * i = state ? ffz(~state) + 1 : 0;
   * psinfo->pr_sname = (i > 5) ? '.' : "RSDTZW"[i];
   *
   * So we replicate that here. From proc_pid_stats(5)
   * R = Running
   * S = Sleeping on uninterrutible wait
   * D = Waiting on uninterruptable disk sleep
   * T = Tracing stop
   * Z = Zombie
   * W = Paging
   */
  switch (process_state) {
  case lldb::StateType::eStateSuspended:
    prpsinfo.pr_sname = 'S';
    prpsinfo.pr_state = 1;
    break;
  case lldb::StateType::eStateStopped:
    [[fallthrough]];
  case lldb::StateType::eStateStepping:
    prpsinfo.pr_sname = 'T';
    prpsinfo.pr_state = 3;
    break;
  case lldb::StateType::eStateUnloaded:
    [[fallthrough]];
  case lldb::StateType::eStateRunning:
    prpsinfo.pr_sname = 'R';
    prpsinfo.pr_state = 0;
    break;
  default:
    break;
  }

  /**
   * pr_flags is left as 0. The values (in linux) are specific
   * to the kernel. We recover them from the proc filesystem
   * but don't put them in ProcessInfo because it would really
   * become very linux specific and the utility here seems pretty
   * dubious
   */

  if (info.EffectiveUserIDIsValid())
    prpsinfo.pr_uid = info.GetUserID();

  if (info.EffectiveGroupIDIsValid())
    prpsinfo.pr_gid = info.GetGroupID();

  if (info.ParentProcessIDIsValid())
    prpsinfo.pr_ppid = info.GetParentProcessID();

  if (info.ProcessGroupIDIsValid())
    prpsinfo.pr_pgrp = info.GetProcessGroupID();

  if (info.ProcessSessionIDIsValid())
    prpsinfo.pr_sid = info.GetProcessSessionID();

  constexpr size_t fname_len = std::extent_v<decltype(prpsinfo.pr_fname)>;
  static_assert(fname_len > 0, "This should always be non zero");
  const llvm::StringRef fname = info.GetNameAsStringRef();
  auto fname_begin = fname.begin();
  std::copy_n(fname_begin, std::min(fname_len, fname.size()),
              prpsinfo.pr_fname);
  prpsinfo.pr_fname[fname_len - 1] = '\0';
  auto args = info.GetArguments();
  auto argentry_iterator = std::begin(args);
  char *psargs = prpsinfo.pr_psargs;
  char *psargs_end = std::end(prpsinfo.pr_psargs);
  while (psargs < psargs_end && argentry_iterator != args.end()) {
    llvm::StringRef argentry = argentry_iterator->ref();
    size_t len =
        std::min<size_t>(std::distance(psargs, psargs_end), argentry.size());
    auto arg_iterator = std::begin(argentry);
    psargs = std::copy_n(arg_iterator, len, psargs);
    if (psargs != psargs_end)
      *(psargs++) = ' ';
    ++argentry_iterator;
  }
  *(psargs - 1) = '\0';
  return prpsinfo;
}

Status ELFLinuxSigInfo::Parse(const DataExtractor &data, const ArchSpec &arch,
                              const lldb::PlatformSP platform_sp,
                              ThreadData &thread_data) {
  if (!platform_sp)
    return Status::FromErrorString("No platform for arch.");
  CompilerType type = platform_sp->GetSiginfoType(arch.GetTriple());
  if (!type.IsValid())
    return Status::FromErrorString("no siginfo_t for platform.");

  auto type_size_or_err = type.GetByteSize(nullptr);
  if (!type_size_or_err)
    return Status::FromError(type_size_or_err.takeError());

  if (data.GetByteSize() < *type_size_or_err)
    return Status::FromErrorString(
        "siginfo note byte size smaller than siginfo_t for platform.");

  lldb::offset_t offset = 0;
  const char *bytes =
      static_cast<const char *>(data.GetData(&offset, *type_size_or_err));
  thread_data.siginfo_bytes = llvm::StringRef(bytes, *type_size_or_err);
  return Status();
}
