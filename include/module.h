#ifndef LLAMA_MODULE_H
#define LLAMA_MODULE_H

#include <module/const_pool.h>
#include <module/class_pool.h>
#include <module/func_pool.h>

#include <cstdint>
#include <cstddef>
#include <vector>

namespace llama {
    class ModuleTree;

    class Module {
    public:
        Module();
        Module(const Module & mod);
        ~Module();

        ClassPool    * get_classes();
        ConstantPool * get_constants();
        FunctionPool * get_functions();

        void dump();
        void build(std::vector<unsigned char> & vec);
    private:
        ClassPool    * classes;
        ConstantPool * consts;
        FunctionPool * funcs;
    };
}

#endif