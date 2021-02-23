// RUN: echo $PATH
// RUN: clang -c -fembed-bitcode -g %S/../Inputs/add.c -o %t-add.o
// RUN: clang -c -fembed-bitcode -g %s -o %t.o
// RUN: mull-ld %t-add.o %t.o

int add(int a, int b);

int main(void) {
    return add(1, 3);
}
