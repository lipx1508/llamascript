#ifndef LLAMA_STD_LIST_H
#define LLAMA_STD_LIST_H

#include <error.h>
#include <value.h>

#include <cstdint>
#include <cstddef>
#include <string>

namespace llama::std_libs {
    class List {
    public:
        List();
        List(const List & list);
        ~List();

        size_t real_idx(int idx);

        void insert(size_t idx, Value & v);
        void insert(size_t idx, Value && v);

        void erase(size_t idx);

        void push(size_t idx, Value & v);
        void push(size_t idx, Value && v);
        void pop(size_t idx, Value & v);
        void pop(size_t idx, Value && v);

        void sort(Value & sort_fn);
    };
}

#endif