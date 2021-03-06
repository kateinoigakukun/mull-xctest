// RUN: %empty-directory(%t)
// RUN: swiftc %s -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/add_sub.swift.out -sdk %target-sdk
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

func rem(lhs: Int, rhs: Int) -> Int {
    return lhs % rhs
}

func add_assign(target: inout Int, value: Int) {
    target += value
}

func sub_assign(target: inout Int, value: Int) {
    target -= value
}

func mul_assign(target: inout Int, value: Int) {
    target *= value
}

func div_assign(target: inout Int, value: Int) {
    target /= value
}

func rem_assign(target: inout Int, value: Int) {
    target %= value
}

// CHECK-DAG: Mutation Point: cxx_add_to_sub {{.*}}/arithmetic.swift:6:16
// CHECK-DAG: Mutation Point: cxx_sub_to_add {{.*}}/arithmetic.swift:10:16
// FIXME: Swift uses `llvm.smul.with.overflow.i64` for mul but there is no corresponding
// instruction like `llvm.sdiv.with.overflow.i64`, so not mutated now.
// FAIL-CHECK-DAG: Mutation Point: cxx_mul_to_div {{.*}}/arithmetic.swift:14:16
// CHECK-DAG: Mutation Point: cxx_div_to_mul {{.*}}/arithmetic.swift:18:16
// CHECK-DAG: Mutation Point: cxx_rem_to_div {{.*}}/arithmetic.swift:22:16

// CHECK-DAG: Mutation Point: cxx_add_assign_to_sub_assign {{.*}}/arithmetic.swift:26:12
// CHECK-DAG: Mutation Point: cxx_sub_assign_to_add_assign {{.*}}/arithmetic.swift:30:12
// FAIL-CHECK-DAG: Mutation Point: cxx_mul_assign_to_div_assign {{.*}}/arithmetic.swift:34:12
// CHECK-DAG: Mutation Point: cxx_div_assign_to_mul_assign {{.*}}/arithmetic.swift:38:12
// CHECK-DAG: Mutation Point: cxx_rem_assign_to_div_assign {{.*}}/arithmetic.swift:42:12
