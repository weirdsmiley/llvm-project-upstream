// NOTE: Assertions have been autogenerated by utils/update_cc_test_checks.py UTC_ARGS: --function-signature --check-globals --filter-out-after "getelem.*kernel" --filter-out "= alloca.*" --include-generated-funcs --replace-value-regex "__omp_offloading_[0-9a-z]+_[0-9a-z]+" "reduction_size[.].+[.]" "pl_cond[.].+[.|,]" --prefix-filecheck-ir-name _ --global-value-regex "\.offload_.*" --global-hex-value-regex ".offload_maptypes.*"
// RUN: %clang_cc1 -verify -fopenmp -fopenmp-targets=powerpc64le-ibm-linux-gnu -x c++ -triple powerpc64le-unknown-unknown -emit-llvm %s -o - | FileCheck %s
// RUN: %clang_cc1 -fopenmp -fopenmp-targets=powerpc64le-ibm-linux-gnu -x c++ -std=c++11 -triple powerpc64le-unknown-unknown -emit-pch -o %t %s
// RUN: %clang_cc1 -fopenmp -fopenmp-targets=powerpc64le-ibm-linux-gnu -x c++ -triple powerpc64le-unknown-unknown -std=c++11 -include-pch %t -verify %s -emit-llvm -o - | FileCheck %s

// expected-no-diagnostics
#ifndef HEADER
#define HEADER

void f1() {
  int *ptr;
  // &ptr, &ptr, sizeof(ptr), TO | FROM | PARAM
  #pragma omp target map(ptr)
    ptr[1] = 6;
}

void f2() {
  int *ptr;
  // &ptr[0], &ptr[0], sizeof(ptr[0]), TO | FROM | PARAM
  #pragma omp target map(*ptr)
    ptr[1] = 6;
}

void f3() {
  int *ptr;
  // &ptr, &ptr, sizeof(ptr), TO | FROM | PARAM
  // &ptr[0], &ptr[0], sizeof(ptr[0]), TO | FROM
  #pragma omp target map(ptr, *ptr)
    ptr[1] = 6;
}

void f4() {
  int *ptr;
  // &ptr[0], &ptr[0], sizeof(ptr[0]), TO | FROM
  // &ptr, &ptr, sizeof(ptr), TO | FROM | PARAM
  #pragma omp target map(*ptr, ptr)
    ptr[2] = 8;
}

#endif
//.
// CHECK: @.offload_sizes = private unnamed_addr constant [1 x i64] [i64 8]
// CHECK: @.offload_maptypes = private unnamed_addr constant [1 x i64] [i64 [[#0x23]]]
// CHECK: @.offload_sizes.1 = private unnamed_addr constant [1 x i64] [i64 4]
// CHECK: @.offload_maptypes.2 = private unnamed_addr constant [1 x i64] [i64 [[#0x23]]]
// CHECK: @.offload_sizes.3 = private unnamed_addr constant [2 x i64] [i64 8, i64 4]
// CHECK: @.offload_maptypes.4 = private unnamed_addr constant [2 x i64] [i64 [[#0x23]], i64 [[#0x3]]]
// CHECK: @.offload_sizes.5 = private unnamed_addr constant [2 x i64] [i64 4, i64 8]
// CHECK: @.offload_maptypes.6 = private unnamed_addr constant [2 x i64] [i64 [[#0x23]], i64 [[#0x3]]]
//.
// CHECK-LABEL: define {{[^@]+}}@_Z2f1v
// CHECK-SAME: () #[[ATTR0:[0-9]+]] {
// CHECK:  entry:
// CHECK:    [[TMP0:%.*]] = getelementptr inbounds [1 x ptr], ptr [[DOTOFFLOAD_BASEPTRS:%.*]], i32 0, i32 0
// CHECK:    store ptr [[PTR:%.*]], ptr [[TMP0]], align 8
// CHECK:    [[TMP1:%.*]] = getelementptr inbounds [1 x ptr], ptr [[DOTOFFLOAD_PTRS:%.*]], i32 0, i32 0
// CHECK:    store ptr [[PTR]], ptr [[TMP1]], align 8
// CHECK:    [[TMP2:%.*]] = getelementptr inbounds [1 x ptr], ptr [[DOTOFFLOAD_MAPPERS:%.*]], i64 0, i64 0
// CHECK:    store ptr null, ptr [[TMP2]], align 8
// CHECK:    [[TMP3:%.*]] = getelementptr inbounds [1 x ptr], ptr [[DOTOFFLOAD_BASEPTRS]], i32 0, i32 0
// CHECK:    [[TMP4:%.*]] = getelementptr inbounds [1 x ptr], ptr [[DOTOFFLOAD_PTRS]], i32 0, i32 0
// CHECK:    [[TMP5:%.*]] = getelementptr inbounds nuw [[STRUCT___TGT_KERNEL_ARGUMENTS:%.*]], ptr [[KERNEL_ARGS:%.*]], i32 0, i32 0
//
//
// CHECK-LABEL: define {{[^@]+}}@{{__omp_offloading_[0-9a-z]+_[0-9a-z]+}}__Z2f1v_l13
// CHECK-SAME: (ptr noundef nonnull align 8 dereferenceable(8) [[PTR:%.*]]) #[[ATTR1:[0-9]+]] {
// CHECK:  entry:
// CHECK:    store ptr [[PTR]], ptr [[PTR_ADDR:%.*]], align 8
// CHECK:    [[TMP0:%.*]] = load ptr, ptr [[PTR_ADDR]], align 8, !nonnull [[META11:![0-9]+]], !align [[META12:![0-9]+]]
// CHECK:    [[TMP1:%.*]] = load ptr, ptr [[TMP0]], align 8
// CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds i32, ptr [[TMP1]], i64 1
// CHECK:    store i32 6, ptr [[ARRAYIDX]], align 4
// CHECK:    ret void
//
//
// CHECK-LABEL: define {{[^@]+}}@_Z2f2v
// CHECK-SAME: () #[[ATTR0]] {
// CHECK:  entry:
// CHECK:    [[TMP0:%.*]] = load ptr, ptr [[PTR:%.*]], align 8
// CHECK:    [[TMP1:%.*]] = load ptr, ptr [[PTR]], align 8
// CHECK:    [[TMP2:%.*]] = load ptr, ptr [[PTR]], align 8
// CHECK:    [[TMP3:%.*]] = getelementptr inbounds [1 x ptr], ptr [[DOTOFFLOAD_BASEPTRS:%.*]], i32 0, i32 0
// CHECK:    store ptr [[TMP1]], ptr [[TMP3]], align 8
// CHECK:    [[TMP4:%.*]] = getelementptr inbounds [1 x ptr], ptr [[DOTOFFLOAD_PTRS:%.*]], i32 0, i32 0
// CHECK:    store ptr [[TMP2]], ptr [[TMP4]], align 8
// CHECK:    [[TMP5:%.*]] = getelementptr inbounds [1 x ptr], ptr [[DOTOFFLOAD_MAPPERS:%.*]], i64 0, i64 0
// CHECK:    store ptr null, ptr [[TMP5]], align 8
// CHECK:    [[TMP6:%.*]] = getelementptr inbounds [1 x ptr], ptr [[DOTOFFLOAD_BASEPTRS]], i32 0, i32 0
// CHECK:    [[TMP7:%.*]] = getelementptr inbounds [1 x ptr], ptr [[DOTOFFLOAD_PTRS]], i32 0, i32 0
// CHECK:    [[TMP8:%.*]] = getelementptr inbounds nuw [[STRUCT___TGT_KERNEL_ARGUMENTS:%.*]], ptr [[KERNEL_ARGS:%.*]], i32 0, i32 0
//
//
// CHECK-LABEL: define {{[^@]+}}@{{__omp_offloading_[0-9a-z]+_[0-9a-z]+}}__Z2f2v_l20
// CHECK-SAME: (ptr noundef [[PTR:%.*]]) #[[ATTR1]] {
// CHECK:  entry:
// CHECK:    store ptr [[PTR]], ptr [[PTR_ADDR:%.*]], align 8
// CHECK:    [[TMP0:%.*]] = load ptr, ptr [[PTR_ADDR]], align 8
// CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds i32, ptr [[TMP0]], i64 1
// CHECK:    store i32 6, ptr [[ARRAYIDX]], align 4
// CHECK:    ret void
//
//
// CHECK-LABEL: define {{[^@]+}}@_Z2f3v
// CHECK-SAME: () #[[ATTR0]] {
// CHECK:  entry:
// CHECK:    [[TMP0:%.*]] = load ptr, ptr [[PTR:%.*]], align 8
// CHECK:    [[TMP1:%.*]] = load ptr, ptr [[PTR]], align 8
// CHECK:    [[TMP2:%.*]] = load ptr, ptr [[PTR]], align 8
// CHECK:    [[TMP3:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_BASEPTRS:%.*]], i32 0, i32 0
// CHECK:    store ptr [[PTR]], ptr [[TMP3]], align 8
// CHECK:    [[TMP4:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_PTRS:%.*]], i32 0, i32 0
// CHECK:    store ptr [[PTR]], ptr [[TMP4]], align 8
// CHECK:    [[TMP5:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_MAPPERS:%.*]], i64 0, i64 0
// CHECK:    store ptr null, ptr [[TMP5]], align 8
// CHECK:    [[TMP6:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_BASEPTRS]], i32 0, i32 1
// CHECK:    store ptr [[TMP1]], ptr [[TMP6]], align 8
// CHECK:    [[TMP7:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_PTRS]], i32 0, i32 1
// CHECK:    store ptr [[TMP2]], ptr [[TMP7]], align 8
// CHECK:    [[TMP8:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_MAPPERS]], i64 0, i64 1
// CHECK:    store ptr null, ptr [[TMP8]], align 8
// CHECK:    [[TMP9:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_BASEPTRS]], i32 0, i32 0
// CHECK:    [[TMP10:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_PTRS]], i32 0, i32 0
// CHECK:    [[TMP11:%.*]] = getelementptr inbounds nuw [[STRUCT___TGT_KERNEL_ARGUMENTS:%.*]], ptr [[KERNEL_ARGS:%.*]], i32 0, i32 0
//
//
// CHECK-LABEL: define {{[^@]+}}@{{__omp_offloading_[0-9a-z]+_[0-9a-z]+}}__Z2f3v_l28
// CHECK-SAME: (ptr noundef [[PTR:%.*]]) #[[ATTR1]] {
// CHECK:  entry:
// CHECK:    store ptr [[PTR]], ptr [[PTR_ADDR:%.*]], align 8
// CHECK:    [[TMP0:%.*]] = load ptr, ptr [[PTR_ADDR]], align 8
// CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds i32, ptr [[TMP0]], i64 1
// CHECK:    store i32 6, ptr [[ARRAYIDX]], align 4
// CHECK:    ret void
//
//
// CHECK-LABEL: define {{[^@]+}}@_Z2f4v
// CHECK-SAME: () #[[ATTR0]] {
// CHECK:  entry:
// CHECK:    [[TMP0:%.*]] = load ptr, ptr [[PTR:%.*]], align 8
// CHECK:    [[TMP1:%.*]] = load ptr, ptr [[PTR]], align 8
// CHECK:    [[TMP2:%.*]] = load ptr, ptr [[PTR]], align 8
// CHECK:    [[TMP3:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_BASEPTRS:%.*]], i32 0, i32 0
// CHECK:    store ptr [[TMP1]], ptr [[TMP3]], align 8
// CHECK:    [[TMP4:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_PTRS:%.*]], i32 0, i32 0
// CHECK:    store ptr [[TMP2]], ptr [[TMP4]], align 8
// CHECK:    [[TMP5:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_MAPPERS:%.*]], i64 0, i64 0
// CHECK:    store ptr null, ptr [[TMP5]], align 8
// CHECK:    [[TMP6:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_BASEPTRS]], i32 0, i32 1
// CHECK:    store ptr [[PTR]], ptr [[TMP6]], align 8
// CHECK:    [[TMP7:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_PTRS]], i32 0, i32 1
// CHECK:    store ptr [[PTR]], ptr [[TMP7]], align 8
// CHECK:    [[TMP8:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_MAPPERS]], i64 0, i64 1
// CHECK:    store ptr null, ptr [[TMP8]], align 8
// CHECK:    [[TMP9:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_BASEPTRS]], i32 0, i32 0
// CHECK:    [[TMP10:%.*]] = getelementptr inbounds [2 x ptr], ptr [[DOTOFFLOAD_PTRS]], i32 0, i32 0
// CHECK:    [[TMP11:%.*]] = getelementptr inbounds nuw [[STRUCT___TGT_KERNEL_ARGUMENTS:%.*]], ptr [[KERNEL_ARGS:%.*]], i32 0, i32 0
//
//
// CHECK-LABEL: define {{[^@]+}}@{{__omp_offloading_[0-9a-z]+_[0-9a-z]+}}__Z2f4v_l36
// CHECK-SAME: (ptr noundef [[PTR:%.*]]) #[[ATTR1]] {
// CHECK:  entry:
// CHECK:    store ptr [[PTR]], ptr [[PTR_ADDR:%.*]], align 8
// CHECK:    [[TMP0:%.*]] = load ptr, ptr [[PTR_ADDR]], align 8
// CHECK:    [[ARRAYIDX:%.*]] = getelementptr inbounds i32, ptr [[TMP0]], i64 2
// CHECK:    store i32 8, ptr [[ARRAYIDX]], align 4
// CHECK:    ret void
//
