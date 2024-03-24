#ifndef LLAMA_MODULE_CONSTPOOL_H
#define LLAMA_MODULE_CONSTPOOL_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

namespace llama {
    class Module;

    class ConstantEntry {
    public:
        enum Type {
            None, 
            Userdata, 
            Int, 
            Float, 
            String, 
        };

        ConstantEntry();
        ConstantEntry(int v);
        ConstantEntry(double v);
        ConstantEntry(std::string str);
        ConstantEntry(const void * ptr, size_t size, Type m_type = Type::Userdata);
        ConstantEntry(const ConstantEntry & entry);
        ~ConstantEntry();

        int         as_int();
        double      as_float();
        std::string as_string();

        Type                         get_type();
        std::vector<unsigned char> & get_data();

        bool operator==(const ConstantEntry & other);

        std::string dump();
    private:
        Type                       type;
        std::vector<unsigned char> data;
    };
    
    class ConstantPool {
    public:
        size_t          get(ConstantEntry entry);
        ConstantEntry * at(size_t idx);
        size_t          size();

        std::string     dump();
        unsigned char * build(size_t * m_size);
    private:
        std::vector<ConstantEntry> entries;

        Module * mod;

        friend Module;
    };
}

#endif