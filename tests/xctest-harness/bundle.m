// RUN: %empty-directory(%t)
// RUN: mkdir -p %t/Foo.xctest/Contents/MacOS/
// RUN: clang -bundle %s -fuse-ld=%mull-xctest-bin/mull-ld -fembed-bitcode -g -o %t/Foo.xctest/Contents/MacOS/Foo -iframework %target-sdk/../../Library/Frameworks -Xlinker -export_dynamic -fobjc-link-runtime -framework XCTest
// RUN: xcrun xctest %t/Foo.xctest
// RUN: mull-xctest %t/Foo.xctest

#import <XCTest/XCTest.h>

@interface Foo : XCTestCase

@end

@implementation Foo

- (void)testExample {
}

@end
