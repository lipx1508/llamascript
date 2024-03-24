#ifndef LLAMA_MODULE_CLASSPOOL_H
#define LLAMA_MODULE_CLASSPOOL_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>

namespace llama {
    class Module;

    class ClassEntry {
    public:
        struct Property {
            std::string name;
            std::string type;
        };
        std::vector<Property>      args;
        std::vector<unsigned char> data;
    };

    class ClassPool {
    public:
        void clear();
        void add(ClassEntry & entry);
        void remove(size_t idx);

        void build(std::vector<unsigned char> & vec);
    private:
        std::vector<ClassEntry> entries;
        
        Module * mod;

        friend Module;
    };
}

#endif