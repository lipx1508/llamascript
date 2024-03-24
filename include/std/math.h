#ifndef LLAMA_STD_MATH_H
#define LLAMA_STD_MATH_H

#include <value.h>

namespace llama::std_libs {
    class Math {
    public:
        static Value sin(Value & v);
        static Value cos(Value & v);
        static Value tan(Value & v);
        static Value asin(Value & v);
        static Value atan(Value & v);
        static Value atan2(Value & x, Value & y);

        static Value exp(Value & v);
        static Value sqrt(Value & v);
        static Value cbrt(Value & v);
        static Value hypot(Value & a, Value & b);

        static Value abs(Value & v);
        static Value floor(Value & v);
        static Value ceil(Value & v);
        static Value round(Value & v);
        static Value truncate(Value & v);

        static Value is_inf(Value & v);
        static Value is_inf(Value & v);
        static Value is_nan(Value & v);
    };
}

#endif