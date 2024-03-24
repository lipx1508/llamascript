#ifndef LLAMA_UTIL_H
#define LLAMA_UTIL_H

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>

#define BOOLALPHA(c) ((c) ? "true" : "false")

namespace llama {
    template <typename T>
    void pack(std::vector<unsigned char> & vec, T val) {
        vec.resize(vec.size() + sizeof(T));
        memcpy(&vec[vec.size() - sizeof(T)], &val, sizeof(T));
    }

    template <typename T>
    void pack(unsigned char * ptr, T val) {
        memcpy(ptr, &val, sizeof(T));
    }

    template <typename T>
    T & unpack(std::vector<unsigned char> & vec, size_t idx = 0) {
        return * reinterpret_cast<T *>(&vec[idx]);
    }

    template <typename T>
    T & unpack(unsigned char * ptr) {
        return * reinterpret_cast<T *>(ptr);
    }
}

#endif