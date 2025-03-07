; NOTE: Assertions have been autogenerated by utils/update_test_checks.py UTC_ARGS: --version 5
; RUN: opt -S --passes=complex-deinterleaving %s --mattr=+sve2 | FileCheck %s

target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128-ni:1-p2:32:8:8:32-ni:2"
target triple = "aarch64-arm-none-linux"

; Ensure that a second reduction-like pattern doesn't override the first
; We don't care what this IR produces, just that it produces something and doesn't cause a crash
define void @reprocessing_crash() #0 {
; CHECK-LABEL: define void @reprocessing_crash
;
entry:
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %entry
  %vec.phi18 = phi <vscale x 2 x double> [ zeroinitializer, %entry ], [ %2, %vector.body ]
  %vec.phi20 = phi <vscale x 2 x double> [ zeroinitializer, %entry ], [ %3, %vector.body ]
  %strided.vec22 = tail call { <vscale x 2 x double>, <vscale x 2 x double> } @llvm.vector.deinterleave2.nxv4f64(<vscale x 4 x double> zeroinitializer)
  %0 = extractvalue { <vscale x 2 x double>, <vscale x 2 x double> } %strided.vec22, 0
  %1 = extractvalue { <vscale x 2 x double>, <vscale x 2 x double> } %strided.vec22, 1
  %2 = fsub <vscale x 2 x double> %vec.phi18, %0
  %3 = fsub <vscale x 2 x double> %vec.phi20, %1
  br i1 false, label %middle.block, label %vector.body

middle.block:                                     ; preds = %vector.body
  %bin.rdx = fadd <vscale x 2 x double> %2, zeroinitializer
  %bin.rdx23 = fadd <vscale x 2 x double> %3, zeroinitializer
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(none)
declare { <vscale x 2 x double>, <vscale x 2 x double> } @llvm.vector.deinterleave2.nxv4f64(<vscale x 4 x double>) #1

attributes #0 = { "target-cpu"="neoverse-v1" }
attributes #1 = { nocallback nofree nosync nounwind willreturn memory(none) }
