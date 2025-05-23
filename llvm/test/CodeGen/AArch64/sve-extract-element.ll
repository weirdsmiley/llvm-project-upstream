; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s | FileCheck %s

target triple = "aarch64-unknown-linux-gnu"

define i8 @test_lane0_16xi8(<vscale x 16 x i8> %a) #0 {
; CHECK-LABEL: test_lane0_16xi8:
; CHECK:       // %bb.0:
; CHECK-NEXT:    fmov w0, s0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 16 x i8> %a, i32 0
  ret i8 %b
}

define i8 @test_lane15_16xi8(<vscale x 16 x i8> %a) #0 {
; CHECK-LABEL: test_lane15_16xi8:
; CHECK:       // %bb.0:
; CHECK-NEXT:    umov w0, v0.b[15]
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 16 x i8> %a, i32 15
  ret i8 %b
}

define i8 @test_lane16_16xi8(<vscale x 16 x i8> %a) #0 {
; CHECK-LABEL: test_lane16_16xi8:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.b, z0.b[16]
; CHECK-NEXT:    fmov w0, s0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 16 x i8> %a, i32 16
  ret i8 %b
}

define i16 @test_lane0_8xi16(<vscale x 8 x i16> %a) #0 {
; CHECK-LABEL: test_lane0_8xi16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    fmov w0, s0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 8 x i16> %a, i32 0
  ret i16 %b
}

define i16 @test_lane7_8xi16(<vscale x 8 x i16> %a) #0 {
; CHECK-LABEL: test_lane7_8xi16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    umov w0, v0.h[7]
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 8 x i16> %a, i32 7
  ret i16 %b
}

define i16 @test_lane8_8xi16(<vscale x 8 x i16> %a) #0 {
; CHECK-LABEL: test_lane8_8xi16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.h, z0.h[8]
; CHECK-NEXT:    fmov w0, s0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 8 x i16> %a, i32 8
  ret i16 %b
}

define i32 @test_lane0_4xi32(<vscale x 4 x i32> %a) #0 {
; CHECK-LABEL: test_lane0_4xi32:
; CHECK:       // %bb.0:
; CHECK-NEXT:    fmov w0, s0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x i32> %a, i32 0
  ret i32 %b
}

define i32 @test_lane3_4xi32(<vscale x 4 x i32> %a) #0 {
; CHECK-LABEL: test_lane3_4xi32:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w0, v0.s[3]
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x i32> %a, i32 3
  ret i32 %b
}

define i32 @test_lane4_4xi32(<vscale x 4 x i32> %a) #0 {
; CHECK-LABEL: test_lane4_4xi32:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.s, z0.s[4]
; CHECK-NEXT:    fmov w0, s0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x i32> %a, i32 4
  ret i32 %b
}

define i64 @test_lane0_2xi64(<vscale x 2 x i64> %a) #0 {
; CHECK-LABEL: test_lane0_2xi64:
; CHECK:       // %bb.0:
; CHECK-NEXT:    fmov x0, d0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x i64> %a, i32 0
  ret i64 %b
}

define i64 @test_lane1_2xi64(<vscale x 2 x i64> %a) #0 {
; CHECK-LABEL: test_lane1_2xi64:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov x0, v0.d[1]
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x i64> %a, i32 1
  ret i64 %b
}

define i64 @test_lane2_2xi64(<vscale x 2 x i64> %a) #0 {
; CHECK-LABEL: test_lane2_2xi64:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.d, z0.d[2]
; CHECK-NEXT:    fmov x0, d0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x i64> %a, i32 2
  ret i64 %b
}

define half @test_lane0_8xf16(<vscale x 8 x half> %a) #0 {
; CHECK-LABEL: test_lane0_8xf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 8 x half> %a, i32 0
  ret half %b
}

define half @test_lane7_8xf16(<vscale x 8 x half> %a) #0 {
; CHECK-LABEL: test_lane7_8xf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.h, z0.h[7]
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 8 x half> %a, i32 7
  ret half %b
}

define half @test_lane8_8xf16(<vscale x 8 x half> %a) #0 {
; CHECK-LABEL: test_lane8_8xf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.h, z0.h[8]
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 8 x half> %a, i32 8
  ret half %b
}

define half @test_lane0_4xf16(<vscale x 4 x half> %a) #0 {
; CHECK-LABEL: test_lane0_4xf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x half> %a, i32 0
  ret half %b
}

define half @test_lane3_4xf16(<vscale x 4 x half> %a) #0 {
; CHECK-LABEL: test_lane3_4xf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.s, z0.s[3]
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x half> %a, i32 3
  ret half %b
}

define half @test_lane4_4xf16(<vscale x 4 x half> %a) #0 {
; CHECK-LABEL: test_lane4_4xf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.s, z0.s[4]
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x half> %a, i32 4
  ret half %b
}

define half @test_lane0_2xf16(<vscale x 2 x half> %a) #0 {
; CHECK-LABEL: test_lane0_2xf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x half> %a, i32 0
  ret half %b
}

define half @test_lane1_2xf16(<vscale x 2 x half> %a) #0 {
; CHECK-LABEL: test_lane1_2xf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.d, z0.d[1]
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x half> %a, i32 1
  ret half %b
}

define half @test_lane2_2xf16(<vscale x 2 x half> %a) #0 {
; CHECK-LABEL: test_lane2_2xf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.d, z0.d[2]
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x half> %a, i32 2
  ret half %b
}

define bfloat @test_lane0_8xbf16(<vscale x 8 x bfloat> %a) #0 {
; CHECK-LABEL: test_lane0_8xbf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 8 x bfloat> %a, i32 0
  ret bfloat %b
}

define bfloat @test_lane7_8xbf16(<vscale x 8 x bfloat> %a) #0 {
; CHECK-LABEL: test_lane7_8xbf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.h, z0.h[7]
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 8 x bfloat> %a, i32 7
  ret bfloat %b
}

define bfloat @test_lane8_8xbf16(<vscale x 8 x bfloat> %a) #0 {
; CHECK-LABEL: test_lane8_8xbf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.h, z0.h[8]
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 8 x bfloat> %a, i32 8
  ret bfloat %b
}

define bfloat @test_lane0_4xbf16(<vscale x 4 x bfloat> %a) #0 {
; CHECK-LABEL: test_lane0_4xbf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x bfloat> %a, i32 0
  ret bfloat %b
}

define bfloat @test_lane3_4xbf16(<vscale x 4 x bfloat> %a) #0 {
; CHECK-LABEL: test_lane3_4xbf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.s, z0.s[3]
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x bfloat> %a, i32 3
  ret bfloat %b
}

define bfloat @test_lane4_4xbf16(<vscale x 4 x bfloat> %a) #0 {
; CHECK-LABEL: test_lane4_4xbf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.s, z0.s[4]
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x bfloat> %a, i32 4
  ret bfloat %b
}

define bfloat @test_lane0_2xbf16(<vscale x 2 x bfloat> %a) #0 {
; CHECK-LABEL: test_lane0_2xbf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x bfloat> %a, i32 0
  ret bfloat %b
}

define bfloat @test_lane1_2xbf16(<vscale x 2 x bfloat> %a) #0 {
; CHECK-LABEL: test_lane1_2xbf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.d, z0.d[1]
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x bfloat> %a, i32 1
  ret bfloat %b
}

define bfloat @test_lane2_2xbf16(<vscale x 2 x bfloat> %a) #0 {
; CHECK-LABEL: test_lane2_2xbf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.d, z0.d[2]
; CHECK-NEXT:    // kill: def $h0 killed $h0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x bfloat> %a, i32 2
  ret bfloat %b
}

define float @test_lane0_4xf32(<vscale x 4 x float> %a) #0 {
; CHECK-LABEL: test_lane0_4xf32:
; CHECK:       // %bb.0:
; CHECK-NEXT:    // kill: def $s0 killed $s0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x float> %a, i32 0
  ret float %b
}

define float @test_lane3_4xf32(<vscale x 4 x float> %a) #0 {
; CHECK-LABEL: test_lane3_4xf32:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.s, z0.s[3]
; CHECK-NEXT:    // kill: def $s0 killed $s0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x float> %a, i32 3
  ret float %b
}

define float @test_lane4_4xf32(<vscale x 4 x float> %a) #0 {
; CHECK-LABEL: test_lane4_4xf32:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.s, z0.s[4]
; CHECK-NEXT:    // kill: def $s0 killed $s0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x float> %a, i32 4
  ret float %b
}

define float @test_lane0_2xf32(<vscale x 2 x float> %a) #0 {
; CHECK-LABEL: test_lane0_2xf32:
; CHECK:       // %bb.0:
; CHECK-NEXT:    // kill: def $s0 killed $s0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x float> %a, i32 0
  ret float %b
}

define float @test_lane1_2xf32(<vscale x 2 x float> %a) #0 {
; CHECK-LABEL: test_lane1_2xf32:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.d, z0.d[1]
; CHECK-NEXT:    // kill: def $s0 killed $s0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x float> %a, i32 1
  ret float %b
}

define float @test_lane2_2xf32(<vscale x 2 x float> %a) #0 {
; CHECK-LABEL: test_lane2_2xf32:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.d, z0.d[2]
; CHECK-NEXT:    // kill: def $s0 killed $s0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x float> %a, i32 2
  ret float %b
}

define double @test_lane0_2xf64(<vscale x 2 x double> %a) #0 {
; CHECK-LABEL: test_lane0_2xf64:
; CHECK:       // %bb.0:
; CHECK-NEXT:    // kill: def $d0 killed $d0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x double> %a, i32 0
  ret double %b
}

define double @test_lane1_2xf64(<vscale x 2 x double> %a) #0 {
; CHECK-LABEL: test_lane1_2xf64:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.d, z0.d[1]
; CHECK-NEXT:    // kill: def $d0 killed $d0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x double> %a, i32 1
  ret double %b
}

define double @test_lane2_2xf64(<vscale x 2 x double> %a) #0 {
; CHECK-LABEL: test_lane2_2xf64:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.d, z0.d[2]
; CHECK-NEXT:    // kill: def $d0 killed $d0 killed $z0
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x double> %a, i32 2
  ret double %b
}

define i8 @test_lanex_16xi8(<vscale x 16 x i8> %a, i32 %x) #0 {
; CHECK-LABEL: test_lanex_16xi8:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, w0
; CHECK-NEXT:    whilels p0.b, xzr, x8
; CHECK-NEXT:    lastb w0, p0, z0.b
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 16 x i8> %a, i32 %x
  ret i8 %b
}

define i16 @test_lanex_8xi16(<vscale x 8 x i16> %a, i32 %x) #0 {
; CHECK-LABEL: test_lanex_8xi16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, w0
; CHECK-NEXT:    whilels p0.h, xzr, x8
; CHECK-NEXT:    lastb w0, p0, z0.h
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 8 x i16> %a, i32 %x
  ret i16 %b
}

define i32 @test_lanex_4xi32(<vscale x 4 x i32> %a, i32 %x) #0 {
; CHECK-LABEL: test_lanex_4xi32:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, w0
; CHECK-NEXT:    whilels p0.s, xzr, x8
; CHECK-NEXT:    lastb w0, p0, z0.s
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x i32> %a, i32 %x
  ret i32 %b
}

define i64 @test_lanex_2xi64(<vscale x 2 x i64> %a, i32 %x) #0 {
; CHECK-LABEL: test_lanex_2xi64:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, w0
; CHECK-NEXT:    whilels p0.d, xzr, x8
; CHECK-NEXT:    lastb x0, p0, z0.d
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x i64> %a, i32 %x
  ret i64 %b
}

define half @test_lanex_8xf16(<vscale x 8 x half> %a, i32 %x) #0 {
; CHECK-LABEL: test_lanex_8xf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, w0
; CHECK-NEXT:    whilels p0.h, xzr, x8
; CHECK-NEXT:    lastb h0, p0, z0.h
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 8 x half> %a, i32 %x
  ret half %b
}

define half @test_lanex_4xf16(<vscale x 4 x half> %a, i32 %x) #0 {
; CHECK-LABEL: test_lanex_4xf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, w0
; CHECK-NEXT:    whilels p0.s, xzr, x8
; CHECK-NEXT:    lastb h0, p0, z0.h
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x half> %a, i32 %x
  ret half %b
}

define half @test_lanex_2xf16(<vscale x 2 x half> %a, i32 %x) #0 {
; CHECK-LABEL: test_lanex_2xf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, w0
; CHECK-NEXT:    whilels p0.d, xzr, x8
; CHECK-NEXT:    lastb h0, p0, z0.h
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x half> %a, i32 %x
  ret half %b
}

define bfloat @test_lanex_8xbf16(<vscale x 8 x bfloat> %a, i32 %x) #0 {
; CHECK-LABEL: test_lanex_8xbf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, w0
; CHECK-NEXT:    whilels p0.h, xzr, x8
; CHECK-NEXT:    lastb h0, p0, z0.h
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 8 x bfloat> %a, i32 %x
  ret bfloat %b
}

define bfloat @test_lanex_4xbf16(<vscale x 4 x bfloat> %a, i32 %x) #0 {
; CHECK-LABEL: test_lanex_4xbf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, w0
; CHECK-NEXT:    whilels p0.s, xzr, x8
; CHECK-NEXT:    lastb h0, p0, z0.h
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x bfloat> %a, i32 %x
  ret bfloat %b
}

define bfloat @test_lanex_2xbf16(<vscale x 2 x bfloat> %a, i32 %x) #0 {
; CHECK-LABEL: test_lanex_2xbf16:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, w0
; CHECK-NEXT:    whilels p0.d, xzr, x8
; CHECK-NEXT:    lastb h0, p0, z0.h
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x bfloat> %a, i32 %x
  ret bfloat %b
}

define float @test_lanex_4xf32(<vscale x 4 x float> %a, i32 %x) #0 {
; CHECK-LABEL: test_lanex_4xf32:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, w0
; CHECK-NEXT:    whilels p0.s, xzr, x8
; CHECK-NEXT:    lastb s0, p0, z0.s
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x float> %a, i32 %x
  ret float %b
}

define float @test_lanex_2xf32(<vscale x 2 x float> %a, i32 %x) #0 {
; CHECK-LABEL: test_lanex_2xf32:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, w0
; CHECK-NEXT:    whilels p0.d, xzr, x8
; CHECK-NEXT:    lastb s0, p0, z0.s
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x float> %a, i32 %x
  ret float %b
}

define double @test_lanex_2xf64(<vscale x 2 x double> %a, i32 %x) #0 {
; CHECK-LABEL: test_lanex_2xf64:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w8, w0
; CHECK-NEXT:    whilels p0.d, xzr, x8
; CHECK-NEXT:    lastb d0, p0, z0.d
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x double> %a, i32 %x
  ret double %b
}

; Deliberately choose an index that is undefined
define i32 @test_undef_lane_4xi32(<vscale x 4 x i32> %a) #0 {
; CHECK-LABEL: test_undef_lane_4xi32:
; CHECK:       // %bb.0:
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x i32> %a, i32 poison
  ret i32 %b
}

define i8 @extract_of_insert_undef_16xi8(i8 %a) #0 {
; CHECK-LABEL: extract_of_insert_undef_16xi8:
; CHECK:       // %bb.0:
; CHECK-NEXT:    ret
  %b = insertelement <vscale x 16 x i8> poison, i8 %a, i32 0
  %c = extractelement <vscale x 16 x i8> %b, i32 0
  ret i8 %c
}

define i8 @extract0_of_insert0_16xi8(<vscale x 16 x i8> %a, i8 %b) #0 {
; CHECK-LABEL: extract0_of_insert0_16xi8:
; CHECK:       // %bb.0:
; CHECK-NEXT:    ret
  %c = insertelement <vscale x 16 x i8> %a, i8 %b, i32 0
  %d = extractelement <vscale x 16 x i8> %c, i32 0
  ret i8 %d
}

define i8 @extract64_of_insert64_16xi8(<vscale x 16 x i8> %a, i8 %b) #0 {
; CHECK-LABEL: extract64_of_insert64_16xi8:
; CHECK:       // %bb.0:
; CHECK-NEXT:    ret
  %c = insertelement <vscale x 16 x i8> %a, i8 %b, i32 64
  %d = extractelement <vscale x 16 x i8> %c, i32 64
  ret i8 %d
}

define i8 @extract_of_insert_diff_lanes_16xi8(<vscale x 16 x i8> %a, i8 %b) #0 {
; CHECK-LABEL: extract_of_insert_diff_lanes_16xi8:
; CHECK:       // %bb.0:
; CHECK-NEXT:    umov w0, v0.b[3]
; CHECK-NEXT:    ret
  %c = insertelement <vscale x 16 x i8> %a, i8 %b, i32 0
  %d = extractelement <vscale x 16 x i8> %c, i32 3
  ret i8 %d
}

define i8 @test_lane0_zero_16xi8(<vscale x 16 x i8> %a) #0 {
; CHECK-LABEL: test_lane0_zero_16xi8:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov w0, wzr
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 16 x i8> zeroinitializer, i32 0
  ret i8 %b
}

; The DAG combiner should fold the extract of a splat to give element zero
; of the splat, i.e. %x. If the index is beyond the end of the scalable
; vector the result is undefined anyway.
define i64 @test_lanex_splat_2xi64(i64 %x, i32 %y) #0 {
; CHECK-LABEL: test_lanex_splat_2xi64:
; CHECK:       // %bb.0:
; CHECK-NEXT:    ret
  %a = insertelement <vscale x 2 x i64> poison, i64 %x, i32 0
  %b = shufflevector <vscale x 2 x i64> %a, <vscale x 2 x i64> poison, <vscale x 2 x i32> zeroinitializer
  %c = extractelement <vscale x 2 x i64> %b, i32 %y
  ret i64 %c
}

define i1 @test_lane0_16xi1(<vscale x 16 x i1> %a) #0 {
; CHECK-LABEL: test_lane0_16xi1:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.b, p0/z, #1 // =0x1
; CHECK-NEXT:    fmov w8, s0
; CHECK-NEXT:    and w0, w8, #0x1
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 16 x i1> %a, i32 0
  ret i1 %b
}

define i1 @test_lane9_8xi1(<vscale x 8 x i1> %a) #0 {
; CHECK-LABEL: test_lane9_8xi1:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.h, p0/z, #1 // =0x1
; CHECK-NEXT:    mov z0.h, z0.h[9]
; CHECK-NEXT:    fmov w8, s0
; CHECK-NEXT:    and w0, w8, #0x1
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 8 x i1> %a, i32 9
  ret i1 %b
}

define i1 @test_last_8xi1(<vscale x 8 x i1> %a) #0 {
; CHECK-LABEL: test_last_8xi1:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov x8, #-1 // =0xffffffffffffffff
; CHECK-NEXT:    mov z0.h, p0/z, #1 // =0x1
; CHECK-NEXT:    whilels p0.h, xzr, x8
; CHECK-NEXT:    lastb w8, p0, z0.h
; CHECK-NEXT:    and w0, w8, #0x1
; CHECK-NEXT:    ret
  %vscale = call i64 @llvm.vscale.i64()
  %shl = shl nuw nsw i64 %vscale, 3
  %idx = add nuw nsw i64 %shl, -1
  %bit = extractelement <vscale x 8 x i1> %a, i64 %idx
  ret i1 %bit
}

define i1 @test_lanex_4xi1(<vscale x 4 x i1> %a, i32 %x) #0 {
; CHECK-LABEL: test_lanex_4xi1:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.s, p0/z, #1 // =0x1
; CHECK-NEXT:    mov w8, w0
; CHECK-NEXT:    whilels p0.s, xzr, x8
; CHECK-NEXT:    lastb w8, p0, z0.s
; CHECK-NEXT:    and w0, w8, #0x1
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 4 x i1> %a, i32 %x
  ret i1 %b
}

define i1 @test_lane4_2xi1(<vscale x 2 x i1> %a) #0 {
; CHECK-LABEL: test_lane4_2xi1:
; CHECK:       // %bb.0:
; CHECK-NEXT:    mov z0.d, p0/z, #1 // =0x1
; CHECK-NEXT:    mov z0.s, z0.s[8]
; CHECK-NEXT:    fmov w8, s0
; CHECK-NEXT:    and w0, w8, #0x1
; CHECK-NEXT:    ret
  %b = extractelement <vscale x 2 x i1> %a, i32 4
  ret i1 %b
}

declare i64 @llvm.vscale.i64()

attributes #0 = { "target-features"="+sve,+bf16" }
