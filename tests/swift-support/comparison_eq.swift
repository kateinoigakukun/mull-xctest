// RUN: %empty-directory(%t)
// RUN: swiftc %s -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/comp.swift.out -sdk %target-sdk
// RUN: mull-dump-mutants %t/comp.swift.out | FileCheck %s

func equal(_ lhs: Int, _ rhs: Int) -> Bool {
    return lhs == rhs
}

func equal_bool(_ v1: Bool, _ v2: Bool) -> Bool {
    return v1 == v2
}

func not_equal(_ lhs: Int, _ rhs: Int) -> Bool {
    return lhs != rhs
}

func not_equal_u64(_ v1: UInt64, _ v2: UInt64) -> Bool {
    return v1 != v2
}

// TODO
func not_equal_u32_u64(_ v1: UInt32, _ v2: UInt64) -> Bool {
    return v1 != v2
}

// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:6:16
// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:10:15
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:14:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:18:15

import Darwin

// RUN: env TEST_swift_eq_to_ne=1 "swift_eq_to_ne:%s:6:16"=1 "swift_eq_to_ne:%s:10:15"=1 %t/comp.swift.out
if getenv("TEST_swift_eq_to_ne") != nil {
    assert(equal(1, 1) == false)
    assert(equal(1, 2) == true)
    assert(equal_bool(true, false) == true)
    assert(equal_bool(true, true) == false)
}

// RUN: env TEST_swift_ne_to_eq=1 "swift_ne_to_eq:%s:14:16"=1 "swift_ne_to_eq:%s:18:15"=1 %t/comp.swift.out
if getenv("TEST_swift_ne_to_eq") != nil {
    assert(not_equal(1, 1) == true)
    assert(not_equal(1, 2) == false)
    assert(not_equal_u64(1, 1) == true)
    assert(not_equal_u64(1, 2) == false)
}
