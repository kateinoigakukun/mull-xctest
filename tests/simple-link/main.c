// RUN: clang -c -fembed-bitcode -g %S/../Inputs/add.c -o %t-add.o
// RUN: clang -c -fembed-bitcode -g %s -o %t.o
// RUN: %mull-xctest-bin/mull-ld %t-add.o %t.o -o %t.out -Xmull --linker=clang
// RUN: %mull-xctest-bin/mull-dump-mutants %t.out | FileCheck %s
// CHECK: Mutation Point: cxx_add_to_sub {{.*}}/add.c:1:34

int add(int a, int b);

int main(void) { return add(1, 3); }
