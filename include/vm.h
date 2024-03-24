#ifndef LLAMA_VM_H
#define LLAMA_VM_H

#include <error.h>
#include <value.h>
#include <module.h>

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <map>

#define REAL_IDX(idx) (size_t)((idx) < 0 ? stack.size() + (idx) : (idx))

#define LLAMA_CFG_NOSTDLIBS  (1 << 0) // Disables inclusion of the standard library
#define LLAMA_CFG_STRICTMODE (1 << 1) // Compilation is stricter
#define LLAMA_CFG_NOMODULES  (1 << 2) // Don't search for modules

namespace llama {
    typedef void (* ExitFn)();                              // Custom function for exiting
    typedef void (* PanicFn)();                             // Custom function for the panic state
    typedef void (* LogFn)(Logger::Type, const char * msg); // Custom function for logging
    typedef void (* FormatFn)(const char * fmt, ...);       // Custom function for formatting

    struct VMConfig {
        ExitFn   exit   = nullptr;
        PanicFn  panic  = nullptr;
        LogFn    log    = nullptr;
        FormatFn format = nullptr;

        short  flags        = 0;
        size_t memory_limit = 1024; // Size is defined in kilobytes
    };

    class VMRunner;

    class VM {
    public:
        VM();
        VM(const VM & vm);
        ~VM();

        void push();
        void push(Value & v);
        void push(Value && v);
        void set_global(std::string name, int value);
        void get_global(std::string name);
        void set_property(std::string name, int idx, int value);
        void get_property(std::string name, int idx);
        void set_index(int idx);
        void get_index(int idx);
        void new_global(std::string name);
        void new_local(std::string name);
        void pop();
        void popn(int n);

        Status call(size_t argc, bool pop = false);
        Status callv(size_t argc, bool pop = false);

        Status load_string(const char * str);
        Status load_file(const char * path);

        Status do_string(const char * str);
        Status do_file(const char * path);

        Value  * get(int idx);
        Module * get_module();

        void dump();
    private:
        Status read(std::string str);

        void exec();

        Logger * log;
        Module * module;

        std::vector<Value>           stack;
        std::map<std::string, Value> globals;

        friend VMRunner;
    };
}

#endif