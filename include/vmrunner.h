#ifndef LLAMA_VMRUNNER_H
#define LLAMA_VMRUNNER_H

#include <error.h>
#include <vm.h>

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace llama {
    class VM;

    class VMRunner {
    public:
        VMRunner(VM * m_vm);
        ~VMRunner();

        Status exec(size_t argc, bool pop = false);

        size_t do_inst(size_t i);
    private:
        VM * vm;
    };
}

#endif