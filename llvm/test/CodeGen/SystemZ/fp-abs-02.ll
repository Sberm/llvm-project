; Test negated floating-point absolute.
;
; RUN: llc < %s -mtriple=s390x-linux-gnu -mcpu=z10 | FileCheck %s
; RUN: llc < %s -mtriple=s390x-linux-gnu -mcpu=z13 | FileCheck %s

; Test f16.
declare half @llvm.fabs.f16(half %f)
define half @f0(half %f) {
; CHECK-LABEL: f0:
; CHECK:      # %bb.0:
; CHECK-NEXT: lndfr %f0, %f0
; CHECK-NEXT: br %r14
  %abs = call half @llvm.fabs.f16(half %f)
  %res = fneg half %abs
  ret half %res
}

; Test f32.
declare float @llvm.fabs.f32(float %f)
define float @f1(float %f) {
; CHECK-LABEL: f1:
; CHECK: lndfr %f0, %f0
; CHECK: br %r14
  %abs = call float @llvm.fabs.f32(float %f)
  %res = fneg float %abs
  ret float %res
}

; Test f64.
declare double @llvm.fabs.f64(double %f)
define double @f2(double %f) {
; CHECK-LABEL: f2:
; CHECK: lndfr %f0, %f0
; CHECK: br %r14
  %abs = call double @llvm.fabs.f64(double %f)
  %res = fneg double %abs
  ret double %res
}

; Test f128.  With the loads and stores, a pure negative-absolute would
; probably be better implemented using an OI on the upper byte.  Do some
; extra processing so that using FPRs is unequivocally better.
declare fp128 @llvm.fabs.f128(fp128 %f)
define void @f3(ptr %ptr, ptr %ptr2) {
; CHECK-LABEL: f3:
; CHECK: lnxbr
; CHECK: dxbr
; CHECK: br %r14
  %orig = load fp128, ptr %ptr
  %abs = call fp128 @llvm.fabs.f128(fp128 %orig)
  %negabs = fneg fp128 %abs
  %op2 = load fp128, ptr %ptr2
  %res = fdiv fp128 %negabs, %op2
  store fp128 %res, ptr %ptr
  ret void
}
