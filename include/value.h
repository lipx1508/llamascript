#ifndef LLAMA_VALUE_H
#define LLAMA_VALUE_H

#include <error.h>

#include <cstdint>
#include <cstddef>
#include <string>

#define IS_REFTYPE(type)    ((type) >= llama::Type::String && (type) <= llama::Type::Userdata)
#define CONV_IDX(size, idx) (size_t)((idx) < 0 ? (size) + (idx) : (idx))

namespace llama {
    enum class Type : unsigned char {
        Null, 
        Bool, 
        Int, 
        Float, 
        String, 
        List, 
        Object, 
        Userdata, 
        Dynamic, 
        Function, 
        Void, // TODO: scrapping this off and detecting void return in another way (?)
    };

    class VM;
    class VMRunner;

    class Value {
    public:
        Value();
        Value(const Value & value);
        ~Value();

        Value(bool v);
        Value(int v);
        Value(double v);
        Value(size_t idx, Type m_type);

        Value _add(Value other);
        Value _sub(Value other);
        Value _mul(Value other);
        Value _div(Value other);
        Value _mod(Value other);
        Value _pow(Value other);
        Value _eq(Value other);
        Value _lt(Value other);
        Value _le(Value other);
        Value _gt(Value other);
        Value _ge(Value other);
        Value _ne(Value other);
        Value _negate();
        Value _promote(); // why does this even exist?
        Value _sizeof();
        Value _lenof();
        Value _typeof();
        Value _instanceof();

        size_t size();

        Value       convert(std::string conv_type);
        Value       as_ref();
        std::string as_string();

        const char * type_str();
    private:
        struct Userdata {
            void * __ptr;
            size_t __size;
        };

        Type type;
        union {
            bool     __bool;
            int      __int;
            double   __float;
            size_t   __idx;
            Userdata __udata;
        } data;

        friend VM;
        friend VMRunner;
    };
}

#endif