// RUN: %empty-directory(%t)
// RUN: swiftc %s -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/logical.swift.out -sdk %target-sdk
// RUN: mull-dump-mutants %t/logical.swift.out | FileCheck %s

func and(_ lhs: Bool, _ rhs: Bool) -> Int {
    if lhs && rhs {
        return 1
    } else {
        return 0
    }
}

func or(_ lhs: Bool, _ rhs: Bool) -> Bool {
    return lhs || rhs
}

// CHECK-DAG: Mutation Point: swift_logical_and_to_or {{.*}}/logical.swift:6:12
