// RUN: %empty-directory(%t)
// RUN: mkdir -p %t/Foo.xctest/Contents/MacOS/
// RUN: swiftc %s -Xlinker -bundle -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/Foo.xctest/Contents/MacOS/Foo -sdk %target-sdk %swiftc-xctest-opts
// RUN: xcrun xctest %t/Foo.xctest
// RUN: mull-xctest %t/Foo.xctest

import XCTest

class Foo: XCTestCase {
    func testExample() {}
}
