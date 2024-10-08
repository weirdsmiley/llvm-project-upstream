; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py UTC_ARGS: --version 4
target datalayout = "E-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v128:128:128-n32:64"
target triple = "powerpc64-unknown-linux-gnu"
; RUN: llc -verify-machineinstrs -mcpu=a2 < %s | FileCheck %s
; RUN: llc -verify-machineinstrs -mcpu=pwr7 < %s | FileCheck %s
; RUN: llc -verify-machineinstrs -mcpu=pwr7 -mattr=-isel < %s | FileCheck --check-prefix=CHECK-NO-ISEL %s

define i64 @test1(i64 %a, i64 %b, i64 %c, i64 %d) {
; CHECK-LABEL: test1:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    cmpld 3, 4
; CHECK-NEXT:    isellt 3, 6, 5
; CHECK-NEXT:    blr
;
; CHECK-NO-ISEL-LABEL: test1:
; CHECK-NO-ISEL:       # %bb.0: # %entry
; CHECK-NO-ISEL-NEXT:    cmpld 3, 4
; CHECK-NO-ISEL-NEXT:    bge 0, .LBB0_2
; CHECK-NO-ISEL-NEXT:  # %bb.1: # %entry
; CHECK-NO-ISEL-NEXT:    mr 5, 6
; CHECK-NO-ISEL-NEXT:  .LBB0_2: # %entry
; CHECK-NO-ISEL-NEXT:    mr 3, 5
; CHECK-NO-ISEL-NEXT:    blr
entry:
	%p = icmp uge i64 %a, %b
	%x = select i1 %p, i64 %c, i64 %d
	ret i64 %x
}

define i32 @test2(i32 %a, i32 %b, i32 %c, i32 %d) {
; CHECK-LABEL: test2:
; CHECK:       # %bb.0: # %entry
; CHECK-NEXT:    cmplw 3, 4
; CHECK-NEXT:    isellt 3, 6, 5
; CHECK-NEXT:    blr
;
; CHECK-NO-ISEL-LABEL: test2:
; CHECK-NO-ISEL:       # %bb.0: # %entry
; CHECK-NO-ISEL-NEXT:    cmplw 3, 4
; CHECK-NO-ISEL-NEXT:    bge 0, .LBB1_2
; CHECK-NO-ISEL-NEXT:  # %bb.1: # %entry
; CHECK-NO-ISEL-NEXT:    mr 5, 6
; CHECK-NO-ISEL-NEXT:  .LBB1_2: # %entry
; CHECK-NO-ISEL-NEXT:    mr 3, 5
; CHECK-NO-ISEL-NEXT:    blr
entry:
	%p = icmp uge i32 %a, %b
	%x = select i1 %p, i32 %c, i32 %d
	ret i32 %x
}

