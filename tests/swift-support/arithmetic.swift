// RUN: %empty-directory(%t)
// RUN: MULL_XCTEST_ARGS="-enable-syntax=true" swiftc %s -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/add_sub.swift.out -sdk %target-sdk
// RUN: mull-dump-mutants %t/add_sub.swift.out | FileCheck %s

func add(lhs: Int, rhs: Int) -> Int {
    return lhs + rhs
}

func sub(lhs: Int, rhs: Int) -> Int {
    return lhs - rhs
}

func mul(lhs: Int, rhs: Int) -> Int {
    return lhs * rhs
}

func div(lhs: Int, rhs: Int) -> Int {
    return lhs / rhs
}

// CHECK: Mutation Point: cxx_add_to_sub {{.*}}/arithmetic.swift:6:16
// CHECK: Mutation Point: cxx_sub_to_add {{.*}}/arithmetic.swift:10:16
// FIXME: Swift uses `llvm.smul.with.overflow.i64` for mul but there is no corresponding
// instruction like `llvm.sdiv.with.overflow.i64`, so not mutated now.
// FAIL-CHECK: Mutation Point: cxx_mul_to_div {{.*}}/arithmetic.swift:14:16
// CHECK: Mutation Point: cxx_div_to_mul {{.*}}/arithmetic.swift:18:16
