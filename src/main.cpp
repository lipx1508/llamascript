/* -=- Includes -=- */
#include <cstdio>
#include <llama.h>

int main(int argc, const char * argv[]) {
    printf("llamaScript 0.1.0 (early preview)\n");
    printf("Copyright (C) 2024 Felipe C. and contributors\n");

    llama::VM * vm = new llama::VM();
    vm->do_file("hello.ls");
    vm->dump();

    delete vm;

    return 0;
}