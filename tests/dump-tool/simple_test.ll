; RUN: clang -c %s -o %t
; RUN: mull-dump-mutants %t | FileCheck %s

; CHECK: Mutation Point: cxx_ne_to_eq /tmp.swift:17:9

target datalayout = "e-m:o-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

@_mull_mutants = private constant [80 x i8] c"cxx_ne_to_eq:/path\00cxx_ne_to_eq\00/tmp_dir\00/tmp.swift\00/tmp_dir\00/tmp.swift\00\00\00\00\11\00\00\00\09", section "__DATA, __mull_mutants", align 1
