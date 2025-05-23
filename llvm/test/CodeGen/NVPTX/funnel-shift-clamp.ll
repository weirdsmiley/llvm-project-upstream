; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py UTC_ARGS: --version 5
; RUN: llc < %s -mtriple=nvptx -mcpu=sm_61 | FileCheck %s
; RUN: llc < %s -mtriple=nvptx64 -mcpu=sm_61 | FileCheck %s

target triple = "nvptx-nvidia-cuda"

declare i32 @llvm.nvvm.fshr.clamp.i32(i32, i32, i32)
declare i32 @llvm.nvvm.fshl.clamp.i32(i32, i32, i32)

define i32 @fshr_clamp_r(i32 %hi, i32 %lo, i32 %n) {
; CHECK-LABEL: fshr_clamp_r(
; CHECK:       {
; CHECK-NEXT:    .reg .b32 %r<5>;
; CHECK-EMPTY:
; CHECK-NEXT:  // %bb.0:
; CHECK-NEXT:    ld.param.b32 %r1, [fshr_clamp_r_param_0];
; CHECK-NEXT:    ld.param.b32 %r2, [fshr_clamp_r_param_1];
; CHECK-NEXT:    ld.param.b32 %r3, [fshr_clamp_r_param_2];
; CHECK-NEXT:    shf.r.clamp.b32 %r4, %r2, %r1, %r3;
; CHECK-NEXT:    st.param.b32 [func_retval0], %r4;
; CHECK-NEXT:    ret;
  %call = call i32 @llvm.nvvm.fshr.clamp.i32(i32 %hi, i32 %lo, i32 %n)
  ret i32 %call
}

define i32 @fshl_clamp_r(i32 %hi, i32 %lo, i32 %n) {
; CHECK-LABEL: fshl_clamp_r(
; CHECK:       {
; CHECK-NEXT:    .reg .b32 %r<5>;
; CHECK-EMPTY:
; CHECK-NEXT:  // %bb.0:
; CHECK-NEXT:    ld.param.b32 %r1, [fshl_clamp_r_param_0];
; CHECK-NEXT:    ld.param.b32 %r2, [fshl_clamp_r_param_1];
; CHECK-NEXT:    ld.param.b32 %r3, [fshl_clamp_r_param_2];
; CHECK-NEXT:    shf.l.clamp.b32 %r4, %r2, %r1, %r3;
; CHECK-NEXT:    st.param.b32 [func_retval0], %r4;
; CHECK-NEXT:    ret;
  %call = call i32 @llvm.nvvm.fshl.clamp.i32(i32 %hi, i32 %lo, i32 %n)
  ret i32 %call
}

define i32 @fshr_clamp_i(i32 %hi, i32 %lo) {
; CHECK-LABEL: fshr_clamp_i(
; CHECK:       {
; CHECK-NEXT:    .reg .b32 %r<4>;
; CHECK-EMPTY:
; CHECK-NEXT:  // %bb.0:
; CHECK-NEXT:    ld.param.b32 %r1, [fshr_clamp_i_param_0];
; CHECK-NEXT:    ld.param.b32 %r2, [fshr_clamp_i_param_1];
; CHECK-NEXT:    shf.r.clamp.b32 %r3, %r2, %r1, 3;
; CHECK-NEXT:    st.param.b32 [func_retval0], %r3;
; CHECK-NEXT:    ret;
  %call = call i32 @llvm.nvvm.fshr.clamp.i32(i32 %hi, i32 %lo, i32 3)
  ret i32 %call
}

define i32 @fshl_clamp_i(i32 %hi, i32 %lo) {
; CHECK-LABEL: fshl_clamp_i(
; CHECK:       {
; CHECK-NEXT:    .reg .b32 %r<4>;
; CHECK-EMPTY:
; CHECK-NEXT:  // %bb.0:
; CHECK-NEXT:    ld.param.b32 %r1, [fshl_clamp_i_param_0];
; CHECK-NEXT:    ld.param.b32 %r2, [fshl_clamp_i_param_1];
; CHECK-NEXT:    shf.l.clamp.b32 %r3, %r2, %r1, 3;
; CHECK-NEXT:    st.param.b32 [func_retval0], %r3;
; CHECK-NEXT:    ret;
  %call = call i32 @llvm.nvvm.fshl.clamp.i32(i32 %hi, i32 %lo, i32 3)
  ret i32 %call
}
