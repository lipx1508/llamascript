#ifndef LLAMA_MODULE_FUNCPOOL_H
#define LLAMA_MODULE_FUNCPOOL_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace llama {
    class Module;
    class VM;

    typedef void (* ExternFunc)(VM *, size_t);

    class FunctionEntry {
    public:
        FunctionEntry();
        FunctionEntry(const FunctionEntry & entry);
        ~FunctionEntry();

        struct Argument {
            std::string field;
            std::string type;
            bool        optional;
        };

        void        set_name(std::string m_name);
        std::string get_name();

        std::vector<unsigned char> & get_data();
        
        int  get_line();
        void set_line(int m_line);

        void     push_arg(Argument arg);
        Argument get_arg(size_t idx);
        size_t   get_argc();

        bool operator==(const FunctionEntry & other);
    private:
        std::string                name;
        std::vector<Argument>      args;
        std::vector<unsigned char> data;

        ExternFunc ext;

        int line;
    };
    
    class FunctionPool {
    public:
        size_t          add(FunctionEntry & entry);
        size_t          get(std::string name);
        bool            has(std::string name);
        FunctionEntry * at(size_t idx);
        size_t          size();

        void build(std::vector<unsigned char> & vec);

        std::string dump(size_t idx, bool show_code = false);
    private:
        std::vector<FunctionEntry> entries;

        Module * mod;

        friend Module;
    };
}

#endif