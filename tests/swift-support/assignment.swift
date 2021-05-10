// RUN: %empty-directory(%t)
// RUN: swiftc %s -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/assignment.swift.out -sdk %target-sdk
// RUN: mull-dump-mutants %t/assignment.swift.out | FileCheck %s

func replace_assignment(_ a: inout Int, _ b: Int) {
  a = b + 100
}

// CHECK: Mutation Point: cxx_assign_const {{.*}}/assignment.swift:6:5

import Darwin

// RUN: env TEST_cxx_assign_const=1 "cxx_assign_const:%s:6:5"=1 %t/assignment.swift.out
if getenv("TEST_cxx_assign_const") != nil {
  var i = 0
  replace_assignment(&i, 20)
  assert(i == 42)
}
