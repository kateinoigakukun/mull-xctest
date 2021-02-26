// RUN: %empty-directory(%t)
// RUN: swiftc %s -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/try.swift.out -sdk %target-sdk

struct MyError: Error {}

func throwingFunc() throws {
    throw MyError()
}

func tryingFunc() throws {
    try throwingFunc()
}
