// RUN: %empty-directory(%t)
// RUN: swiftc %s -tools-directory %mull-xctest-bin -embed-bitcode -g -o %t/comp.swift.out -sdk %target-sdk
// RUN: mull-dump-mutants %t/comp.swift.out | FileCheck %s

// ==================== PRIMITIVE TYPES ====================
func equal_Int(_ lhs: Int, _ rhs: Int) -> Bool {
    return lhs == rhs
}

func equal_Int8(_ lhs: Int8, _ rhs: Int8) -> Bool {
    return lhs == rhs
}

func equal_Int16(_ lhs: Int16, _ rhs: Int16) -> Bool {
    return lhs == rhs
}

func equal_Int32(_ lhs: Int32, _ rhs: Int32) -> Bool {
    return lhs == rhs
}

func equal_Int64(_ lhs: Int64, _ rhs: Int64) -> Bool {
    return lhs == rhs
}

func equal_UInt(_ lhs: UInt, _ rhs: UInt) -> Bool {
    return lhs == rhs
}

func equal_UInt8(_ lhs: UInt8, _ rhs: UInt8) -> Bool {
    return lhs == rhs
}

func equal_UInt16(_ lhs: UInt16, _ rhs: UInt16) -> Bool {
    return lhs == rhs
}

func equal_UInt32(_ lhs: UInt32, _ rhs: UInt32) -> Bool {
    return lhs == rhs
}

func equal_UInt64(_ lhs: UInt64, _ rhs: UInt64) -> Bool {
    return lhs == rhs
}

func equal_Bool(_ lhs: Bool, _ rhs: Bool) -> Bool {
    return lhs == rhs
}

func not_equal_Int(_ lhs: Int, _ rhs: Int) -> Bool {
    return lhs != rhs
}

func not_equal_Int8(_ lhs: Int8, _ rhs: Int8) -> Bool {
    return lhs != rhs
}

func not_equal_Int16(_ lhs: Int16, _ rhs: Int16) -> Bool {
    return lhs != rhs
}

func not_equal_Int32(_ lhs: Int32, _ rhs: Int32) -> Bool {
    return lhs != rhs
}

func not_equal_Int64(_ lhs: Int64, _ rhs: Int64) -> Bool {
    return lhs != rhs
}

func not_equal_UInt(_ lhs: UInt, _ rhs: UInt) -> Bool {
    return lhs != rhs
}

func not_equal_UInt8(_ lhs: UInt8, _ rhs: UInt8) -> Bool {
    return lhs != rhs
}

func not_equal_UInt16(_ lhs: UInt16, _ rhs: UInt16) -> Bool {
    return lhs != rhs
}

func not_equal_UInt32(_ lhs: UInt32, _ rhs: UInt32) -> Bool {
    return lhs != rhs
}

func not_equal_UInt64(_ lhs: UInt64, _ rhs: UInt64) -> Bool {
    return lhs != rhs
}

func not_equal_Bool(_ lhs: Bool, _ rhs: Bool) -> Bool {
    return lhs != rhs
}


// ==================== COMPLEX TYPES ====================
func equal_u32_u64(_ lhs: UInt32, _ rhs: UInt64) -> Bool {
    return lhs == rhs
}

func not_equal_u32_u64(_ lhs: UInt32, _ rhs: UInt64) -> Bool {
    return lhs != rhs
}

func equal_i32_u64(_ lhs: Int32, _ rhs: UInt64) -> Bool {
    return lhs == rhs
}

func not_equal_i32_u64(_ lhs: Int32, _ rhs: UInt64) -> Bool {
    return lhs != rhs
}

func equal_u64_i32(_ lhs: UInt64, _ rhs: Int32) -> Bool {
    return lhs == rhs
}

func not_equal_u64_i32(_ lhs: UInt64, _ rhs: Int32) -> Bool {
    return lhs != rhs
}

func equal_i32_u32(_ lhs: Int32, _ rhs: UInt32) -> Bool {
    return lhs == rhs
}

func not_equal_i32_u32(_ lhs: Int32, _ rhs: UInt32) -> Bool {
    return lhs != rhs
}

func equal_generic<T: Equatable>(_ lhs: T, _ rhs: T) -> Bool {
    return lhs == rhs
}

// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:7:16
// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:11:16
// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:15:16
// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:19:16
// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:23:16
// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:27:16
// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:31:16
// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:35:16
// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:39:16
// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:43:16
// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:47:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:51:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:55:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:59:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:63:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:67:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:71:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:75:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:79:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:83:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:87:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:91:16

// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:97:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:101:16
// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:105:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:109:16
// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:113:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:117:16
// CHECK-DAG: Mutation Point: swift_eq_to_ne {{.*}}/comparison_eq.swift:121:16
// CHECK-DAG: Mutation Point: swift_ne_to_eq {{.*}}/comparison_eq.swift:125:16

import Darwin

// RUN: env TEST_swift_eq_to_ne=1 \
// RUN:   "swift_eq_to_ne:%s:7:16"=1   "swift_eq_to_ne:%s:11:16"=1 "swift_eq_to_ne:%s:15:16"=1 \
// RUN:   "swift_eq_to_ne:%s:19:16"=1  "swift_eq_to_ne:%s:23:16"=1 "swift_eq_to_ne:%s:27:16"=1 \
// RUN:   "swift_eq_to_ne:%s:31:16"=1  "swift_eq_to_ne:%s:35:16"=1 "swift_eq_to_ne:%s:39:16"=1 \
// RUN:   "swift_eq_to_ne:%s:43:16"=1  "swift_eq_to_ne:%s:47:16"=1 \
// RUN:   "swift_eq_to_ne:%s:97:16"=1  "swift_eq_to_ne:%s:105:16"=1 \
// RUN:   "swift_eq_to_ne:%s:113:16"=1 "swift_eq_to_ne:%s:121:16"=1 \
// RUN:   %t/comp.swift.out
if getenv("TEST_swift_eq_to_ne") != nil {
    assert(equal_Int(1, 1) == false)
    assert(equal_Int(1, 2) == true)
    assert(equal_Int8(1, 1) == false)
    assert(equal_Int8(1, 2) == true)
    assert(equal_Int16(1, 1) == false)
    assert(equal_Int16(1, 2) == true)
    assert(equal_Int32(1, 1) == false)
    assert(equal_Int32(1, 2) == true)
    assert(equal_Int64(1, 1) == false)
    assert(equal_Int64(1, 2) == true)
    assert(equal_UInt(1, 1) == false)
    assert(equal_UInt(1, 2) == true)
    assert(equal_UInt8(1, 1) == false)
    assert(equal_UInt8(1, 2) == true)
    assert(equal_UInt16(1, 1) == false)
    assert(equal_UInt16(1, 2) == true)
    assert(equal_UInt32(1, 1) == false)
    assert(equal_UInt32(1, 2) == true)
    assert(equal_UInt64(1, 1) == false)
    assert(equal_UInt64(1, 2) == true)
    assert(equal_Bool(true, true) == false)
    assert(equal_Bool(true, false) == true)

    assert(equal_u32_u64(1, 1) == false)
    assert(equal_u32_u64(1, 2) == true)
    assert(equal_i32_u64(1, 1) == false)
    assert(equal_i32_u64(1, 2) == true)
    assert(equal_u64_i32(1, 1) == false)
    assert(equal_u64_i32(1, 2) == true)
    assert(equal_i32_u32(1, 1) == false)
    assert(equal_i32_u32(1, 2) == true)
}

// RUN: env TEST_swift_ne_to_eq=1 \
// RUN:   "swift_ne_to_eq:%s:51:16"=1 "swift_ne_to_eq:%s:55:16"=1 "swift_ne_to_eq:%s:59:16"=1 \
// RUN:   "swift_ne_to_eq:%s:63:16"=1 "swift_ne_to_eq:%s:67:16"=1 "swift_ne_to_eq:%s:71:16"=1 \
// RUN:   "swift_ne_to_eq:%s:75:16"=1 "swift_ne_to_eq:%s:79:16"=1 "swift_ne_to_eq:%s:83:16"=1 \
// RUN:   "swift_ne_to_eq:%s:87:16"=1 "swift_ne_to_eq:%s:91:16"=1 \
// RUN:   "swift_ne_to_eq:%s:101:16"=1 "swift_ne_to_eq:%s:109:16"=1 \
// RUN:   "swift_ne_to_eq:%s:117:16"=1 "swift_ne_to_eq:%s:125:16"=1 \
// RUN:   %t/comp.swift.out
if getenv("TEST_swift_ne_to_eq") != nil {
    assert(not_equal_Int(1, 1) == true)
    assert(not_equal_Int(1, 2) == false)
    assert(not_equal_Int8(1, 1) == true)
    assert(not_equal_Int8(1, 2) == false)
    assert(not_equal_Int16(1, 1) == true)
    assert(not_equal_Int16(1, 2) == false)
    assert(not_equal_Int32(1, 1) == true)
    assert(not_equal_Int32(1, 2) == false)
    assert(not_equal_Int64(1, 1) == true)
    assert(not_equal_Int64(1, 2) == false)
    assert(not_equal_UInt(1, 1) == true)
    assert(not_equal_UInt(1, 2) == false)
    assert(not_equal_UInt8(1, 1) == true)
    assert(not_equal_UInt8(1, 2) == false)
    assert(not_equal_UInt16(1, 1) == true)
    assert(not_equal_UInt16(1, 2) == false)
    assert(not_equal_UInt32(1, 1) == true)
    assert(not_equal_UInt32(1, 2) == false)
    assert(not_equal_UInt64(1, 1) == true)
    assert(not_equal_UInt64(1, 2) == false)
    assert(not_equal_Bool(true, true) == true)
    assert(not_equal_Bool(true, false) == false)

    assert(not_equal_u32_u64(1, 1) == true)
    assert(not_equal_u32_u64(1, 2) == false)
    assert(not_equal_i32_u64(1, 1) == true)
    assert(not_equal_i32_u64(1, 2) == false)
    assert(not_equal_u64_i32(1, 1) == true)
    assert(not_equal_u64_i32(1, 2) == false)
    assert(not_equal_i32_u32(1, 1) == true)
    assert(not_equal_i32_u32(1, 2) == false)
}
