// RUN: clang -c -fembed-bitcode -g %S/../Inputs/add.c -o %t-add.o
// RUN: clang -c -fembed-bitcode -g %s -o %t.o
// RUN: %mull-xctest-bin/mull-ld %t-add.o %t.o -o %t.out -Xmull --linker=clang
// RUN: %mull-xctest-bin/mull-dump-mutants %t.out

int add(int a, int b);

int main(void) { return add(1, 3); }
