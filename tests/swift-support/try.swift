// RUN: %empty-directory(%t)
// RUN: swiftc %s -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/try.swift.out -sdk %target-sdk
// no mutation
// RUN: mull-dump-mutants %t/try.swift.out | FileCheck %s
// CHECK: no mutant info found
// CHECK-NOT: cxx_eq_to_ne /<compiler-generated>:0:0

struct MyError: Error {}

func throwingFunc() throws {
    throw MyError()
}

func tryingFunc() throws {
// CHECK-NOT: cxx_ne_to_eq {{.+}}try.swift:15:9
    try throwingFunc()
}
