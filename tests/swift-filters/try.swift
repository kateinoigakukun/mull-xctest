// RUN: %empty-directory(%t)
// RUN: swiftc %s -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/try.swift.out -sdk %target-sdk
// RUN: mull-dump-mutants %t/try.swift.out | FileCheck %s

// CHECK-NOT: cxx_eq_to_ne /<compiler-generated>:0:0
// CHECK-NOT: cxx_ne_to_eq {{.*}}try.swift:11:9

struct MyError: Error {}

func throwingFunc() throws {
    throw MyError()
}

func tryingFunc() throws {
    try throwingFunc()
}
