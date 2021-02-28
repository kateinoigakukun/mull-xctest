// RUN: %empty-directory(%t)
// RUN: MULL_XCTEST_ARGS="-enable-syntax=false" swiftc %s -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/add_sub.swift.out -sdk %target-sdk
// RUN: mull-dump-mutants %t/add_sub.swift.out | FileCheck %s
// RUN: MULL_XCTEST_ARGS="-enable-syntax=true" swiftc %s -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/add_sub.swift.out -sdk %target-sdk
// RUN: mull-dump-mutants %t/add_sub.swift.out | FileCheck %s

func add(lhs: Int, rhs: Int) -> Int {
    return lhs + rhs
}

// CHECK: Mutation Point: cxx_add_to_sub {{.*}}/add_sub.swift:8:16
