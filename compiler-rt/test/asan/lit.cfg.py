# -*- Python -*-

import os
import platform
import re
import shlex

import lit.formats


def get_required_attr(config, attr_name):
    attr_value = getattr(config, attr_name, None)
    if attr_value is None:
        lit_config.fatal(
            "No attribute %r in test configuration! You may need to run "
            "tests from your build directory or add this attribute "
            "to lit.site.cfg.py " % attr_name
        )
    return attr_value

# Setup config name.
config.name = "AddressSanitizer" + config.name_suffix

# Platform-specific default ASAN_OPTIONS for lit tests.
default_asan_opts = list(config.default_sanitizer_opts)

# On Darwin, leak checking is not enabled by default. Enable on macOS
# tests to prevent regressions.
# Currently, detect_leaks for asan tests only work on Intel MacOS.
if (
    config.target_os == "Darwin"
    and config.apple_platform == "osx"
    and config.target_arch == "x86_64"
):
    default_asan_opts += ["detect_leaks=1"]

default_asan_opts_str = ":".join(default_asan_opts)
if default_asan_opts_str:
    config.environment["ASAN_OPTIONS"] = default_asan_opts_str
    default_asan_opts_str += ":"
config.substitutions.append(
    ("%env_asan_opts=", "env ASAN_OPTIONS=" + default_asan_opts_str)
)

# Setup source root.
config.test_source_root = os.path.dirname(__file__)

if config.target_os not in ["FreeBSD", "NetBSD"]:
    libdl_flag = "-ldl"
else:
    libdl_flag = ""

# GCC-ASan doesn't link in all the necessary libraries automatically, so
# we have to do it ourselves.
if config.compiler_id == "GNU":
    extra_link_flags = ["-pthread", "-lstdc++", libdl_flag]
else:
    extra_link_flags = []

# Setup default compiler flags used with -fsanitize=address option.
# FIXME: Review the set of required flags and check if it can be reduced.
target_cflags = [get_required_attr(config, "target_cflags")] + extra_link_flags
target_cxxflags = config.cxx_mode_flags + target_cflags
clang_asan_static_cflags = (
    [
        "-fsanitize=address",
        "-mno-omit-leaf-frame-pointer",
        "-fno-omit-frame-pointer",
        "-fno-optimize-sibling-calls",
    ]
    + config.debug_info_flags
    + target_cflags
)
if config.target_arch == "s390x":
    clang_asan_static_cflags.append("-mbackchain")
clang_asan_static_cxxflags = config.cxx_mode_flags + clang_asan_static_cflags

target_is_msvc = bool(re.match(r".*-windows-msvc$", config.target_triple))

asan_dynamic_flags = []
if config.asan_dynamic:
    asan_dynamic_flags = ["-shared-libasan"]
    if platform.system() == "Windows" and target_is_msvc:
        # On MSVC target, we need to simulate "clang-cl /MD" on the clang driver side.
        asan_dynamic_flags += [
            "-D_MT",
            "-D_DLL",
            "-Wl,-nodefaultlib:libcmt,-defaultlib:msvcrt,-defaultlib:oldnames",
        ]
    elif platform.system() == "FreeBSD":
        # On FreeBSD, we need to add -pthread to ensure pthread functions are available.
        asan_dynamic_flags += ["-pthread"]
    config.available_features.add("asan-dynamic-runtime")
else:
    config.available_features.add("asan-static-runtime")
clang_asan_cflags = clang_asan_static_cflags + asan_dynamic_flags
clang_asan_cxxflags = clang_asan_static_cxxflags + asan_dynamic_flags

# Add win32-(static|dynamic)-asan features to mark tests as passing or failing
# in those modes. lit doesn't support logical feature test combinations.
if platform.system() == "Windows":
    if config.asan_dynamic:
        win_runtime_feature = "win32-dynamic-asan"
    else:
        win_runtime_feature = "win32-static-asan"
    config.available_features.add(win_runtime_feature)


def build_invocation(compile_flags, with_lto=False):
    lto_flags = []
    if with_lto and config.lto_supported:
        lto_flags += config.lto_flags

    return " " + " ".join([config.clang] + lto_flags + compile_flags) + " "


config.substitutions.append(("%clang ", build_invocation(target_cflags)))
config.substitutions.append(("%clangxx ", build_invocation(target_cxxflags)))
config.substitutions.append(("%clang_asan ", build_invocation(clang_asan_cflags)))
config.substitutions.append(("%clangxx_asan ", build_invocation(clang_asan_cxxflags)))
config.substitutions.append(
    ("%clang_asan_lto ", build_invocation(clang_asan_cflags, True))
)
config.substitutions.append(
    ("%clangxx_asan_lto ", build_invocation(clang_asan_cxxflags, True))
)
if config.asan_dynamic:
    if config.target_os in ["Linux", "FreeBSD", "NetBSD", "SunOS"]:
        shared_libasan_path = os.path.join(
            config.compiler_rt_libdir,
            "libclang_rt.asan{}.so".format(config.target_suffix),
        )
    elif config.target_os == "Darwin":
        shared_libasan_path = os.path.join(
            config.compiler_rt_libdir,
            "libclang_rt.asan_{}_dynamic.dylib".format(config.apple_platform),
        )
    elif config.target_os == "Windows":
        shared_libasan_path = os.path.join(
            config.compiler_rt_libdir,
            "clang_rt.asan_dynamic-{}.lib".format(config.target_suffix),
        )
    else:
        lit_config.warning(
            "%shared_libasan substitution not set but dynamic ASan is available."
        )
        shared_libasan_path = None

    if shared_libasan_path is not None:
        config.substitutions.append(("%shared_libasan", shared_libasan_path))
    config.substitutions.append(
        ("%clang_asan_static ", build_invocation(clang_asan_static_cflags))
    )
    config.substitutions.append(
        ("%clangxx_asan_static ", build_invocation(clang_asan_static_cxxflags))
    )

if platform.system() == "Windows":
    # MSVC-specific tests might also use the clang-cl.exe driver.
    if target_is_msvc:
        clang_cl_cxxflags = (
            [
                "-WX",
                "-D_HAS_EXCEPTIONS=0",
            ]
            + config.debug_info_flags
            + target_cflags
        )
        if config.compiler_id != "MSVC":
            clang_cl_cxxflags = ["-Wno-deprecated-declarations"] + clang_cl_cxxflags
        clang_cl_asan_cxxflags = ["-fsanitize=address"] + clang_cl_cxxflags
        if config.asan_dynamic:
            clang_cl_asan_cxxflags.append("-MD")

        clang_cl_invocation = build_invocation(clang_cl_cxxflags)
        clang_cl_invocation = clang_cl_invocation.replace("clang.exe", "clang-cl.exe")
        config.substitutions.append(("%clang_cl ", clang_cl_invocation))

        clang_cl_asan_invocation = build_invocation(clang_cl_asan_cxxflags)
        clang_cl_asan_invocation = clang_cl_asan_invocation.replace(
            "clang.exe", "clang-cl.exe"
        )
        config.substitutions.append(("%clang_cl_asan ", clang_cl_asan_invocation))
        config.substitutions.append(("%clang_cl_nocxx_asan ", clang_cl_asan_invocation))
        config.substitutions.append(("%Od", "-Od"))
        config.substitutions.append(("%Fe", "-Fe"))
        config.substitutions.append(("%LD", "-LD"))
        config.substitutions.append(("%MD", "-MD"))
        config.substitutions.append(("%MT", "-MT"))
        config.substitutions.append(("%Gw", "-Gw"))
        config.substitutions.append(("%Oy-", "-Oy-"))

        base_lib = os.path.join(
            config.compiler_rt_libdir, "clang_rt.asan%%s%s.lib" % config.target_suffix
        )
        config.substitutions.append(("%asan_lib", base_lib % "_dynamic"))
        if config.asan_dynamic:
            config.substitutions.append(
                ("%asan_thunk", base_lib % "_dynamic_runtime_thunk")
            )
        else:
            config.substitutions.append(
                ("%asan_thunk", base_lib % "_static_runtime_thunk")
            )
        config.substitutions.append(("%asan_cxx_lib", base_lib % "_cxx"))
        config.substitutions.append(
            ("%asan_dynamic_runtime_thunk", base_lib % "_dynamic_runtime_thunk")
        )
        config.substitutions.append(
            ("%asan_static_runtime_thunk", base_lib % "_static_runtime_thunk")
        )
        config.substitutions.append(("%asan_dll_thunk", base_lib % "_dll_thunk"))
    else:
        # To make some of these tests work on MinGW target without changing their
        # behaviour for MSVC target, substitute clang-cl flags with gcc-like ones.
        config.substitutions.append(("%clang_cl ", build_invocation(target_cxxflags)))
        config.substitutions.append(
            ("%clang_cl_asan ", build_invocation(clang_asan_cxxflags))
        )
        config.substitutions.append(
            ("%clang_cl_nocxx_asan ", build_invocation(clang_asan_cflags))
        )
        config.substitutions.append(("%Od", "-O0"))
        config.substitutions.append(("%Fe", "-o"))
        config.substitutions.append(("%LD", "-shared"))
        config.substitutions.append(("%MD", ""))
        config.substitutions.append(("%MT", ""))
        config.substitutions.append(("%Gw", "-fdata-sections"))
        config.substitutions.append(("%Oy-", "-fno-omit-frame-pointer"))

# FIXME: De-hardcode this path.
asan_source_dir = os.path.join(
    get_required_attr(config, "compiler_rt_src_root"), "lib", "asan"
)
python_exec = shlex.quote(get_required_attr(config, "python_executable"))
# Setup path to asan_symbolize.py script.
asan_symbolize = os.path.join(asan_source_dir, "scripts", "asan_symbolize.py")
if not os.path.exists(asan_symbolize):
    lit_config.fatal("Can't find script on path %r" % asan_symbolize)
config.substitutions.append(
    ("%asan_symbolize", python_exec + " " + asan_symbolize + " ")
)
# Setup path to sancov.py script.
sanitizer_common_source_dir = os.path.join(
    get_required_attr(config, "compiler_rt_src_root"), "lib", "sanitizer_common"
)
sancov = os.path.join(sanitizer_common_source_dir, "scripts", "sancov.py")
if not os.path.exists(sancov):
    lit_config.fatal("Can't find script on path %r" % sancov)
config.substitutions.append(("%sancov ", python_exec + " " + sancov + " "))

# Determine kernel bitness
if config.host_arch.find("64") != -1 and not config.android:
    kernel_bits = "64"
else:
    kernel_bits = "32"

config.substitutions.append(
    ("CHECK-%kernel_bits", ("CHECK-kernel-" + kernel_bits + "-bits"))
)

config.substitutions.append(("%libdl", libdl_flag))

config.available_features.add("asan-" + config.bits + "-bits")

# Fast unwinder doesn't work with Thumb
if not config.arm_thumb:
    config.available_features.add("fast-unwinder-works")

# Turn on leak detection on 64-bit Linux.
leak_detection_android = (
    config.android
    and "android-thread-properties-api" in config.available_features
    and (config.target_arch in ["x86_64", "i386", "i686", "aarch64"])
)
leak_detection_linux = (
    (config.target_os == "Linux")
    and (not config.android)
    and (config.target_arch in ["x86_64", "i386", "riscv64", "loongarch64"])
)
leak_detection_mac = (
    (config.target_os == "Darwin")
    and (config.apple_platform == "osx")
    and (config.target_arch == "x86_64")
)
leak_detection_netbsd = (config.target_os == "NetBSD") and (
    config.target_arch in ["x86_64", "i386"]
)
if (
    leak_detection_android
    or leak_detection_linux
    or leak_detection_mac
    or leak_detection_netbsd
):
    config.available_features.add("leak-detection")

# Add the RT libdir to PATH directly so that we can successfully run the gtest
# binary to list its tests.
if config.target_os == "Windows":
    os.environ["PATH"] = os.path.pathsep.join(
        [config.compiler_rt_libdir, os.environ.get("PATH", "")]
    )

# msvc needs to be instructed where the compiler-rt libraries are
if config.compiler_id == "MSVC":
    config.environment["LIB"] = os.path.pathsep.join(
        [config.compiler_rt_libdir, config.environment.get("LIB", "")]
    )

# Default test suffixes.
config.suffixes = [".c", ".cpp"]

if config.target_os == "Darwin":
    config.suffixes.append(".mm")

if config.target_os == "Windows":
    config.substitutions.append(("%fPIC", ""))
    config.substitutions.append(("%fPIE", ""))
    config.substitutions.append(("%pie", ""))
else:
    config.substitutions.append(("%fPIC", "-fPIC"))
    config.substitutions.append(("%fPIE", "-fPIE"))
    config.substitutions.append(("%pie", "-pie"))

# Only run the tests on supported OSs.
if config.target_os not in ["Linux", "Darwin", "FreeBSD", "SunOS", "Windows", "NetBSD"]:
    config.unsupported = True

if not config.parallelism_group:
    config.parallelism_group = "shadow-memory"

if config.target_os == "NetBSD":
    config.substitutions.insert(0, ("%run", config.netbsd_noaslr_prefix))
