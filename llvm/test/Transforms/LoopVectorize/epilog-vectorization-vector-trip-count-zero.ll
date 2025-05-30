; NOTE: Assertions have been autogenerated by utils/update_test_checks.py UTC_ARGS: --version 5
; RUN: opt -p loop-vectorize -force-vector-width=2 -epilogue-vectorization-force-VF=2 -S %s | FileCheck %s

target datalayout = "E-m:e-i1:8:16-i8:8:16-i64:64-f128:64-v128:64-a:8:16-n32:64"

; Test case for https://github.com/llvm/llvm-project/issues/122558.
define void @vector_trip_count_0_as_btc_is_all_1(ptr %dst) #0 {
; CHECK-LABEL: define void @vector_trip_count_0_as_btc_is_all_1(
; CHECK-SAME: ptr [[DST:%.*]]) {
; CHECK-NEXT:  [[ENTRY:.*]]:
; CHECK-NEXT:    br label %[[LOOP:.*]]
; CHECK:       [[LOOP]]:
; CHECK-NEXT:    [[IV:%.*]] = phi i32 [ 0, %[[ENTRY]] ], [ [[IV_NEXT:%.*]], %[[LOOP]] ]
; CHECK-NEXT:    [[IV_NEXT]] = add i32 [[IV]], -1
; CHECK-NEXT:    [[GEP:%.*]] = getelementptr inbounds i32, ptr [[DST]], i32 [[IV]]
; CHECK-NEXT:    store i32 [[IV_NEXT]], ptr [[GEP]], align 4
; CHECK-NEXT:    [[EC:%.*]] = icmp eq i32 [[IV_NEXT]], 0
; CHECK-NEXT:    br i1 [[EC]], label %[[EXIT:.*]], label %[[LOOP]]
; CHECK:       [[EXIT]]:
; CHECK-NEXT:    ret void
;
entry:
  br label %loop

loop:
  %iv = phi i32 [ 0, %entry ], [ %iv.next, %loop ]
  %iv.next = add i32 %iv, -1
  %gep = getelementptr inbounds i32, ptr %dst, i32 %iv
  store i32 %iv.next, ptr %gep, align 4
  %ec = icmp eq i32 %iv.next, 0
  br i1 %ec, label %exit, label %loop

exit:
  ret void
}
