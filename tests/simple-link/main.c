// RUN: clang -c -fembed-bitcode -g %S/../Inputs/add.c -o %t-add.o
// RUN: clang -c -fembed-bitcode -g %s -o %t.o
// RUN: MULL_XCTEST_ARGS="--linker clang" mull-ld %t-add.o %t.o -o %t.out
// RUN: mull-dump-mutants %t.out

int add(int a, int b);

int main(void) { return add(1, 3); }
