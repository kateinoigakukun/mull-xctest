// RUN: %empty-directory(%t)
// RUN: MULL_XCTEST_ARGS="-enable-syntax=true" swiftc %s -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/comp.swift.out -sdk %target-sdk
// RUN: mull-dump-mutants %t/comp.swift.out | FileCheck %s

func equal(_ lhs: Int, _ rhs: Int) -> Bool {
    return lhs == rhs
}

func not_equal(_ lhs: Int, _ rhs: Int) -> Bool {
    return lhs != rhs
}

func less_than(_ lhs: Int, _ rhs: Int) -> Bool {
    return lhs < rhs
}

func greater_than(_ lhs: Int, _ rhs: Int) -> Bool {
    return lhs > rhs
}

func less_or_equal(_ lhs: Int, _ rhs: Int) -> Bool {
    return lhs <= rhs
}

func greater_or_equal(_ lhs: Int, _ rhs: Int) -> Bool {
    return lhs >= rhs
}

// CHECK-DAG: Mutation Point: cxx_eq_to_ne {{.*}}/comparison.swift:6:16
// CHECK-DAG: Mutation Point: cxx_ne_to_eq {{.*}}/comparison.swift:10:16
// CHECK-DAG: Mutation Point: cxx_lt_to_ge {{.*}}/comparison.swift:14:16
// CHECK-DAG: Mutation Point: cxx_gt_to_le {{.*}}/comparison.swift:18:16
// CHECK-DAG: Mutation Point: cxx_le_to_gt {{.*}}/comparison.swift:22:16
// CHECK-DAG: Mutation Point: cxx_ge_to_lt {{.*}}/comparison.swift:26:16

import Darwin

// RUN: env TEST_cxx_eq_to_ne=1 "cxx_eq_to_ne:%s:6:16"=1 %t/comp.swift.out
if getenv("TEST_cxx_eq_to_ne") != nil {
    assert(equal(1, 1) == false)
    assert(equal(1, 2) == true)
}

// RUN: env TEST_cxx_ne_to_eq=1 "cxx_ne_to_eq:%s:10:16"=1 %t/comp.swift.out
if getenv("TEST_cxx_ne_to_eq") != nil {
    assert(not_equal(1, 1) == true)
    assert(not_equal(1, 2) == false)
}

// RUN: env TEST_cxx_lt_to_ge=1 "cxx_lt_to_ge:%s:14:16"=1 %t/comp.swift.out
if getenv("TEST_cxx_lt_to_ge") != nil {
    // < -> >=
    assert(less_than(1, 2) == false)
    assert(less_than(2, 1) == true)
    assert(less_than(1, 1) == true)
}

// RUN: env TEST_cxx_gt_to_le=1 "cxx_gt_to_le:%s:18:16"=1 %t/comp.swift.out
if getenv("TEST_cxx_gt_to_le") != nil {
    // > -> <=
    assert(greater_than(2, 1) == false)
    assert(greater_than(1, 2) == true)
    assert(greater_than(1, 1) == true)
}

// RUN: env TEST_cxx_le_to_gt=1 "cxx_le_to_gt:%s:22:16"=1 %t/comp.swift.out
if getenv("TEST_cxx_le_to_gt") != nil {
    // <= -> >
    assert(less_or_equal(1, 2) == false)
    assert(less_or_equal(2, 1) == true)
    assert(less_or_equal(1, 1) == false)
}

// RUN: env TEST_cxx_ge_to_lt=1 "cxx_ge_to_lt:%s:26:16"=1 %t/comp.swift.out
if getenv("TEST_cxx_ge_to_lt") != nil {
    // >= -> <
    assert(greater_or_equal(2, 1) == false)
    assert(greater_or_equal(1, 2) == true)
    assert(greater_or_equal(1, 1) == false)
}
