/* -=============
     Includes
   =============- */

#include <error.h>
#include <module.h>
#include <ir.h>
#include <util.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>

/* -=================
     Module class
   =================- */

/* -=- (Con/des)tructors -=- */
llama::Module::Module() {
    classes = new ClassPool();
    consts  = new ConstantPool();
    funcs   = new FunctionPool();

    classes->mod = this;
    consts->mod  = this;
    funcs->mod   = this;
}

llama::Module::Module(const Module & mod) {
    if (this != &mod) {
        classes = new ClassPool();
        consts  = new ConstantPool();
        funcs   = new FunctionPool();

        classes->mod = mod.classes->mod;
        consts->mod  = mod.consts->mod;
        funcs->mod   = mod.funcs->mod;
    }
}

llama::Module::~Module() {
    delete classes;
    delete consts;
    delete funcs;
}

/* -=- (S/g)etters -=- */
llama::ClassPool * llama::Module::get_classes() {
    return classes;
}

llama::ConstantPool * llama::Module::get_constants() {
    return consts;
}

llama::FunctionPool * llama::Module::get_functions() {
    return funcs;
}

/* -=- Base functions -=- */
void llama::Module::dump() {
    printf("-- CPOOL DUMP (%zu entries) --\n%s\n", consts->size(), consts->dump().c_str());
    printf("-- FUNCTIONS DUMP (%zu entries) --\n", funcs->size());
    for (size_t i = 0; i < funcs->size(); ++i) {
        printf("function %zu = %s\n", i, funcs->dump(i, true).c_str());
    }
}