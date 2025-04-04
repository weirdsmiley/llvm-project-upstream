; RUN: llc < %s | FileCheck %s

target datalayout = "e-m:x-p:32:32-i64:64-f80:32-n8:16:32-a:0:32-S32"
target triple = "i686-pc-windows-msvc"

define void @try_except() #0 personality ptr @_except_handler3 {
entry:
  %__exception_code = alloca i32, align 4
  call void (...) @llvm.localescape(ptr %__exception_code)
  invoke void @f(i32 1) #3
          to label %invoke.cont unwind label %catch.dispatch

catch.dispatch:                                   ; preds = %entry
  %cs1 = catchswitch within none [label %__except.ret] unwind to caller

__except.ret:                                     ; preds = %catch.dispatch
  %0 = catchpad within %cs1 [ptr @try_except_filter_catchall]
  catchret from %0 to label %__except

__except:                                         ; preds = %__except.ret
  call void @f(i32 2)
  br label %__try.cont

__try.cont:                                       ; preds = %__except, %invoke.cont
  call void @f(i32 3)
  ret void

invoke.cont:                                      ; preds = %entry
  br label %__try.cont
}

; CHECK-LABEL: _try_except:
;     Store state #0
; CHECK: movl $0, -[[state:[0-9]+]](%ebp)
; CHECK: pushl $1
; CHECK: calll _f
; CHECK: movl $-1, -[[state]](%ebp)
; CHECK: pushl $3
; CHECK: calll _f
; CHECK: retl

;   __except
; CHECK: movl $-1, -[[state]](%ebp)
; CHECK: pushl $2
; CHECK: calll _f

; CHECK: .section        .xdata,"dr"
; CHECK: L__ehtable$try_except:
; CHECK:         .long   -1                          # ToState
; CHECK:         .long   _try_except_filter_catchall # Filter
; CHECK:         .long   LBB0_1

define internal i32 @try_except_filter_catchall() #0 {
entry:
  %0 = call ptr @llvm.frameaddress(i32 1)
  %1 = call ptr @llvm.eh.recoverfp(ptr @try_except, ptr %0)
  %2 = call ptr @llvm.localrecover(ptr @try_except, ptr %1, i32 0)
  %3 = getelementptr inbounds i8, ptr %0, i32 -20
  %4 = load ptr, ptr %3, align 4
  %5 = getelementptr inbounds { ptr, ptr }, ptr %4, i32 0, i32 0
  %6 = load ptr, ptr %5, align 4
  %7 = load i32, ptr %6, align 4
  store i32 %7, ptr %2, align 4
  ret i32 1
}

define void @nested_exceptions() #0 personality ptr @_except_handler3 {
entry:
  %__exception_code = alloca i32, align 4
  call void (...) @llvm.localescape(ptr %__exception_code)
  invoke void @crash() #3
          to label %__try.cont unwind label %catch.dispatch

catch.dispatch:                                   ; preds = %entry
  %cs1 = catchswitch within none [label %__except.ret] unwind label %catch.dispatch.11

__except.ret:                                     ; preds = %catch.dispatch
  %0 = catchpad within %cs1 [ptr @nested_exceptions_filter_catchall]
  catchret from %0 to label %__try.cont

__try.cont:                                       ; preds = %entry, %__except.ret
  invoke void @crash() #3
          to label %__try.cont.9 unwind label %catch.dispatch.5

catch.dispatch.5:                                 ; preds = %__try.cont
  %cs2 = catchswitch within none [label %__except.ret.7] unwind label %catch.dispatch.11

__except.ret.7:                                   ; preds = %catch.dispatch.5
  %1 = catchpad within %cs2 [ptr @nested_exceptions_filter_catchall]
  catchret from %1 to label %__try.cont.9

__try.cont.9:                                     ; preds = %__try.cont, %__except.ret.7
  invoke void @crash() #3
          to label %__try.cont.15 unwind label %catch.dispatch.11

catch.dispatch.11:                                ; preds = %catchendblock, %catchendblock.6, %__try.cont.9
  %cs3 = catchswitch within none [label %__except.ret.13] unwind label %catch.dispatch.17

__except.ret.13:                                  ; preds = %catch.dispatch.11
  %2 = catchpad within %cs3 [ptr @nested_exceptions_filter_catchall]
  catchret from %2 to label %__try.cont.15

__try.cont.15:                                    ; preds = %__try.cont.9, %__except.ret.13
  invoke void @crash() #3
          to label %__try.cont.35 unwind label %catch.dispatch.17

catch.dispatch.17:                                ; preds = %catchendblock.12, %__try.cont.15
  %cs4 = catchswitch within none [label %__except.ret.19] unwind to caller

__except.ret.19:                                  ; preds = %catch.dispatch.17
  %3 = catchpad within %cs4 [ptr @nested_exceptions_filter_catchall]
  catchret from %3 to label %__except.20

__except.20:                                      ; preds = %__except.ret.19
  invoke void @crash() #3
          to label %__try.cont.27 unwind label %catch.dispatch.23

catch.dispatch.23:                                ; preds = %__except.20
  %cs5 = catchswitch within none [label %__except.ret.25] unwind to caller

__except.ret.25:                                  ; preds = %catch.dispatch.23
  %4 = catchpad within %cs5 [ptr @nested_exceptions_filter_catchall]
  catchret from %4 to label %__try.cont.27

__try.cont.27:                                    ; preds = %__except.20, %__except.ret.25
  invoke void @crash() #3
          to label %__try.cont.35 unwind label %catch.dispatch.30

catch.dispatch.30:                                ; preds = %__try.cont.27
  %cs6 = catchswitch within none [label %__except.ret.32] unwind to caller

__except.ret.32:                                  ; preds = %catch.dispatch.30
  %5 = catchpad within %cs6 [ptr @nested_exceptions_filter_catchall]
  catchret from %5 to label %__try.cont.35

__try.cont.35:                                    ; preds = %__try.cont.15, %__try.cont.27, %__except.ret.32
  ret void
}

; This table is equivalent to the one produced by MSVC, even if it isn't in
; quite the same order.

; CHECK-LABEL: _nested_exceptions:
; CHECK: L__ehtable$nested_exceptions:
; CHECK:         .long   -1
; CHECK:         .long   _nested_exceptions_filter_catchall
; CHECK:         .long   LBB
; CHECK:         .long   0
; CHECK:         .long   _nested_exceptions_filter_catchall
; CHECK:         .long   LBB
; CHECK:         .long   1
; CHECK:         .long   _nested_exceptions_filter_catchall
; CHECK:         .long   LBB
; CHECK:         .long   1
; CHECK:         .long   _nested_exceptions_filter_catchall
; CHECK:         .long   LBB
; CHECK:         .long   -1
; CHECK:         .long   _nested_exceptions_filter_catchall
; CHECK:         .long   LBB
; CHECK:         .long   -1
; CHECK:         .long   _nested_exceptions_filter_catchall
; CHECK:         .long   LBB

declare void @crash() #0

define internal i32 @nested_exceptions_filter_catchall() #0 {
entry:
  %0 = call ptr @llvm.frameaddress(i32 1)
  %1 = call ptr @llvm.eh.recoverfp(ptr @nested_exceptions, ptr %0)
  %2 = call ptr @llvm.localrecover(ptr @nested_exceptions, ptr %1, i32 0)
  %3 = getelementptr inbounds i8, ptr %0, i32 -20
  %4 = load ptr, ptr %3, align 4
  %5 = getelementptr inbounds { ptr, ptr }, ptr %4, i32 0, i32 0
  %6 = load ptr, ptr %5, align 4
  %7 = load i32, ptr %6, align 4
  store i32 %7, ptr %2, align 4
  ret i32 1
}

define void @code_in_catchpad() #0 personality ptr @_except_handler3 {
entry:
  invoke void @f(i32 1) #3
          to label %__except unwind label %catch.dispatch

catch.dispatch:                                   ; preds = %entry
  %cs1 = catchswitch within none [label %__except.ret] unwind to caller

__except.ret:                                     ; preds = %catch.dispatch
  %0 = catchpad within %cs1 [ptr @try_except_filter_catchall]
  call void @f(i32 2) [ "funclet"(token %0) ]
  catchret from %0 to label %__except

__except:
  ret void
}

; CHECK-LABEL: _code_in_catchpad:
; CHECK: # %__except.ret
; CHECK-NEXT:         $ehgcr_4_1:
; CHECK-NEXT:         movl    -24(%ebp), %esp
; CHECK-NEXT:         addl    $12, %ebp
; CHECK-NEXT:         movl    $-1, -16(%ebp)
; CHECK-NEXT:         pushl   $2
; CHECK-NEXT:         calll   _f


; Function Attrs: nounwind readnone
declare ptr @llvm.frameaddress(i32) #1

; Function Attrs: nounwind readnone
declare ptr @llvm.eh.recoverfp(ptr, ptr) #1

; Function Attrs: nounwind readnone
declare ptr @llvm.localrecover(ptr, ptr, i32) #1

declare void @f(i32) #0

declare i32 @_except_handler3(...)

; Function Attrs: nounwind
declare void @llvm.localescape(...) #2

attributes #0 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "frame-pointer"="none" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-realign-stack" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }
attributes #2 = { nounwind }
attributes #3 = { noinline }
