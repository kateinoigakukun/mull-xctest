// RUN: %empty-directory(%t)
// RUN: swiftc %s -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/assignment.swift.out -sdk %target-sdk
// RUN: mull-dump-mutants %t/comp.swift.out | FileCheck %s

func replace_assignment(_ a: inout Int, _ b: Int) {
  a = b + 100
}

import Darwin

// RUN: env TEST_cxx_assign_const=1 "cxx_lt_to_ge:%s:15:16"=1 %t/comp.swift.out
if getenv("TEST_cxx_assign_const") != nil {
  var i = 0
  replace_assignment(&i, 20)
  assert(i == 42)
}
