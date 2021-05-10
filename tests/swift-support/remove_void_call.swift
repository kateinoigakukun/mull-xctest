// RUN: %empty-directory(%t)
// RUN: swiftc %s -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/remove_void_call.swift.out -sdk %target-sdk
// RUN: mull-dump-mutants %t/remove_void_call.swift.out | FileCheck %s

var globalState: Int = 0

func void_function() {
  globalState = 1
}

func call_void_function() -> Int {
  void_function()
  return 0
}

// CHECK: Mutation Point: cxx_remove_void_call {{.*}}/remove_void_call.swift:12:3

import Darwin

// RUN: env TEST_cxx_remove_void_call=1 "cxx_remove_void_call:%s:12:3"=1 %t/remove_void_call.swift.out
if getenv("TEST_cxx_remove_void_call") != nil {
  _ = call_void_function()
  assert(globalState == 0)
}
