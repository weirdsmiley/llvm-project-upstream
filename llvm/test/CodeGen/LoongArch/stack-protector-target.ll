; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py UTC_ARGS: --version 5
; RUN: llc -mtriple=loongarch32-linux-gnu < %s | FileCheck -check-prefix=LINUX32 %s
; RUN: llc -mtriple=loongarch64-linux-gnu < %s | FileCheck -check-prefix=LINUX64 %s
; RUN: llc -mtriple=loongarch32-unknown-openbsd < %s | FileCheck -check-prefix=OPENBSD32 %s
; RUN: llc -mtriple=loongarch64-unknown-openbsd < %s | FileCheck -check-prefix=OPENBSD64 %s

define void @func() sspreq nounwind {
; LINUX32-LABEL: func:
; LINUX32:       # %bb.0:
; LINUX32-NEXT:    addi.w $sp, $sp, -16
; LINUX32-NEXT:    st.w $ra, $sp, 12 # 4-byte Folded Spill
; LINUX32-NEXT:    st.w $fp, $sp, 8 # 4-byte Folded Spill
; LINUX32-NEXT:    pcalau12i $fp, %pc_hi20(__stack_chk_guard)
; LINUX32-NEXT:    ld.w $a0, $fp, %pc_lo12(__stack_chk_guard)
; LINUX32-NEXT:    st.w $a0, $sp, 4
; LINUX32-NEXT:    addi.w $a0, $sp, 0
; LINUX32-NEXT:    bl capture
; LINUX32-NEXT:    ld.w $a0, $fp, %pc_lo12(__stack_chk_guard)
; LINUX32-NEXT:    ld.w $a1, $sp, 4
; LINUX32-NEXT:    bne $a0, $a1, .LBB0_2
; LINUX32-NEXT:  # %bb.1:
; LINUX32-NEXT:    ld.w $fp, $sp, 8 # 4-byte Folded Reload
; LINUX32-NEXT:    ld.w $ra, $sp, 12 # 4-byte Folded Reload
; LINUX32-NEXT:    addi.w $sp, $sp, 16
; LINUX32-NEXT:    ret
; LINUX32-NEXT:  .LBB0_2:
; LINUX32-NEXT:    bl __stack_chk_fail
;
; LINUX64-LABEL: func:
; LINUX64:       # %bb.0:
; LINUX64-NEXT:    addi.d $sp, $sp, -32
; LINUX64-NEXT:    st.d $ra, $sp, 24 # 8-byte Folded Spill
; LINUX64-NEXT:    st.d $fp, $sp, 16 # 8-byte Folded Spill
; LINUX64-NEXT:    pcalau12i $fp, %pc_hi20(__stack_chk_guard)
; LINUX64-NEXT:    ld.d $a0, $fp, %pc_lo12(__stack_chk_guard)
; LINUX64-NEXT:    st.d $a0, $sp, 8
; LINUX64-NEXT:    addi.d $a0, $sp, 4
; LINUX64-NEXT:    pcaddu18i $ra, %call36(capture)
; LINUX64-NEXT:    jirl $ra, $ra, 0
; LINUX64-NEXT:    ld.d $a0, $fp, %pc_lo12(__stack_chk_guard)
; LINUX64-NEXT:    ld.d $a1, $sp, 8
; LINUX64-NEXT:    bne $a0, $a1, .LBB0_2
; LINUX64-NEXT:  # %bb.1:
; LINUX64-NEXT:    ld.d $fp, $sp, 16 # 8-byte Folded Reload
; LINUX64-NEXT:    ld.d $ra, $sp, 24 # 8-byte Folded Reload
; LINUX64-NEXT:    addi.d $sp, $sp, 32
; LINUX64-NEXT:    ret
; LINUX64-NEXT:  .LBB0_2:
; LINUX64-NEXT:    pcaddu18i $ra, %call36(__stack_chk_fail)
; LINUX64-NEXT:    jirl $ra, $ra, 0
;
; OPENBSD32-LABEL: func:
; OPENBSD32:       # %bb.0:
; OPENBSD32-NEXT:    addi.w $sp, $sp, -16
; OPENBSD32-NEXT:    st.w $ra, $sp, 12 # 4-byte Folded Spill
; OPENBSD32-NEXT:    st.w $fp, $sp, 8 # 4-byte Folded Spill
; OPENBSD32-NEXT:    pcalau12i $fp, %pc_hi20(__guard_local)
; OPENBSD32-NEXT:    ld.w $a0, $fp, %pc_lo12(__guard_local)
; OPENBSD32-NEXT:    st.w $a0, $sp, 4
; OPENBSD32-NEXT:    addi.w $a0, $sp, 0
; OPENBSD32-NEXT:    bl capture
; OPENBSD32-NEXT:    ld.w $a0, $fp, %pc_lo12(__guard_local)
; OPENBSD32-NEXT:    ld.w $a1, $sp, 4
; OPENBSD32-NEXT:    bne $a0, $a1, .LBB0_2
; OPENBSD32-NEXT:  # %bb.1: # %SP_return
; OPENBSD32-NEXT:    ld.w $fp, $sp, 8 # 4-byte Folded Reload
; OPENBSD32-NEXT:    ld.w $ra, $sp, 12 # 4-byte Folded Reload
; OPENBSD32-NEXT:    addi.w $sp, $sp, 16
; OPENBSD32-NEXT:    ret
; OPENBSD32-NEXT:  .LBB0_2: # %CallStackCheckFailBlk
; OPENBSD32-NEXT:    pcalau12i $a0, %pc_hi20(.LSSH)
; OPENBSD32-NEXT:    addi.w $a0, $a0, %pc_lo12(.LSSH)
; OPENBSD32-NEXT:    bl __stack_smash_handler
;
; OPENBSD64-LABEL: func:
; OPENBSD64:       # %bb.0:
; OPENBSD64-NEXT:    addi.d $sp, $sp, -32
; OPENBSD64-NEXT:    st.d $ra, $sp, 24 # 8-byte Folded Spill
; OPENBSD64-NEXT:    st.d $fp, $sp, 16 # 8-byte Folded Spill
; OPENBSD64-NEXT:    pcalau12i $fp, %pc_hi20(__guard_local)
; OPENBSD64-NEXT:    ld.d $a0, $fp, %pc_lo12(__guard_local)
; OPENBSD64-NEXT:    st.d $a0, $sp, 8
; OPENBSD64-NEXT:    addi.d $a0, $sp, 4
; OPENBSD64-NEXT:    pcaddu18i $ra, %call36(capture)
; OPENBSD64-NEXT:    jirl $ra, $ra, 0
; OPENBSD64-NEXT:    ld.d $a0, $fp, %pc_lo12(__guard_local)
; OPENBSD64-NEXT:    ld.d $a1, $sp, 8
; OPENBSD64-NEXT:    bne $a0, $a1, .LBB0_2
; OPENBSD64-NEXT:  # %bb.1: # %SP_return
; OPENBSD64-NEXT:    ld.d $fp, $sp, 16 # 8-byte Folded Reload
; OPENBSD64-NEXT:    ld.d $ra, $sp, 24 # 8-byte Folded Reload
; OPENBSD64-NEXT:    addi.d $sp, $sp, 32
; OPENBSD64-NEXT:    ret
; OPENBSD64-NEXT:  .LBB0_2: # %CallStackCheckFailBlk
; OPENBSD64-NEXT:    pcalau12i $a0, %pc_hi20(.LSSH)
; OPENBSD64-NEXT:    addi.d $a0, $a0, %pc_lo12(.LSSH)
; OPENBSD64-NEXT:    pcaddu18i $ra, %call36(__stack_smash_handler)
; OPENBSD64-NEXT:    jirl $ra, $ra, 0
  %alloca = alloca i32, align 4
  call void @capture(ptr %alloca)
  ret void
}

declare void @capture(ptr)
