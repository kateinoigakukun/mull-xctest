// RUN: %empty-directory(%t)
// RUN: swiftc %s -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/boundary.swift.out -sdk %target-sdk
// RUN: mull-dump-mutants %t/boundary.swift.out | FileCheck %s

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

// CHECK-DAG: Mutation Point: cxx_lt_to_le {{.*}}/boundary.swift:6:16
// CHECK-DAG: Mutation Point: cxx_gt_to_ge {{.*}}/boundary.swift:10:16
// CHECK-DAG: Mutation Point: cxx_le_to_lt {{.*}}/boundary.swift:14:16
// CHECK-DAG: Mutation Point: cxx_ge_to_gt {{.*}}/boundary.swift:18:16

import Darwin

// RUN: env TEST_cxx_lt_to_le=1 "cxx_lt_to_le:%s:6:16"=1 %t/boundary.swift.out
if getenv("TEST_cxx_lt_to_le") != nil {
    // < -> <=
    assert(less_than(1, 2) == true)
    assert(less_than(2, 1) == false)
    assert(less_than(1, 1) == true)
}

// RUN: env TEST_cxx_gt_to_ge=1 "cxx_gt_to_ge:%s:10:16"=1 %t/boundary.swift.out
if getenv("TEST_cxx_gt_to_ge") != nil {
    // > -> >=
    assert(greater_than(2, 1) == true)
    assert(greater_than(1, 2) == false)
    assert(greater_than(1, 1) == true)
}

// RUN: env TEST_cxx_le_to_lt=1 "cxx_le_to_lt:%s:14:16"=1 %t/boundary.swift.out
if getenv("TEST_cxx_le_to_lt") != nil {
    // <= -> <
    assert(less_or_equal(1, 2) == true)
    assert(less_or_equal(2, 1) == false)
    assert(less_or_equal(1, 1) == false)
}

// RUN: env TEST_cxx_ge_to_gt=1 "cxx_ge_to_gt:%s:18:16"=1 %t/boundary.swift.out
if getenv("TEST_cxx_ge_to_gt") != nil {
    // >= -> >
    assert(greater_or_equal(2, 1) == true)
    assert(greater_or_equal(1, 2) == false)
    assert(greater_or_equal(1, 1) == false)
}
