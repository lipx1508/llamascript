/* -=============
     Includes
   =============- */

#include <error.h>
#include <value.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>

/* -==============
     Internals
   ==============- */

namespace llama {

}

/* -================
     Value class
   ================- */

/* -=- (Con/des)tructors -=- */
llama::Value::Value() {
    type = Type::Null;
    data = { 0 };
}

llama::Value::Value(const Value & value) {
    if (this != &value) {
        type = value.type;
        data = value.data;
        if (IS_REFTYPE(type)) {
            auto & tdata = data.__udata;
            auto & vdata = value.data.__udata;

            tdata.__size = vdata.__size;
            tdata.__ptr  = malloc(vdata.__size);
            memcpy(tdata.__ptr, vdata.__ptr, vdata.__size);
        }
    }
}

llama::Value::~Value() {
    if (IS_REFTYPE(type)) {
        free(data.__udata.__ptr);
        data.__udata.__ptr = nullptr;
    }
    type = Type::Null;
}

llama::Value::Value(bool v) {
    type        = Type::Bool;
    data.__bool = v;
}

llama::Value::Value(int v) {
    type       = Type::Int;
    data.__int = v;
}

llama::Value::Value(double v) {
    type         = Type::Float;
    data.__float = v;
}

llama::Value::Value(size_t idx, Type m_type) {
    type       = m_type;
    data.__idx = idx;
}

/* -=- Metafunctions -=- */
llama::Value llama::Value::_add(Value other) {
    Value val = * this;
    if (val.type != other.type) return Value();

    if (val.type == Type::Int) {
        val.data.__int += other.data.__int;
    } else if (val.type == Type::Float) {
        val.data.__float += other.data.__float;
    }

    return val;
}

llama::Value llama::Value::_sub(Value other) {
    Value val = * this;
    if (val.type != other.type) return Value();

    if (val.type == Type::Int) {
        val.data.__int -= other.data.__int;
    } else if (val.type == Type::Float) {
        val.data.__float -= other.data.__float;
    }
    
    return val;
}

llama::Value llama::Value::_mul(Value other) {
    Value val = * this;
    if (val.type != other.type) return Value();

    if (val.type == Type::Int) {
        val.data.__int *= other.data.__int;
    } else if (val.type == Type::Float) {
        val.data.__float *= other.data.__float;
    }
    
    return val;
}

llama::Value llama::Value::_div(Value other) {
    Value val = * this;
    if (val.type != other.type) return Value();

    if (val.type == Type::Int) {
        val.data.__int /= other.data.__int;
    } else if (val.type == Type::Float) {
        val.data.__float /= other.data.__float;
    }
    
    return val;
}

llama::Value llama::Value::_mod(Value other) {
    Value val = * this;
    if (val.type != other.type) return Value();

    if (val.type == Type::Int) {
        val.data.__int %= other.data.__int;
    } else if (val.type == Type::Float) {
        val.data.__float = fmod(val.data.__float, other.data.__float);
    }
    
    return val;
}

llama::Value llama::Value::_pow(Value other) {
    Value val = * this;
    if (val.type == Type::Int && other.type == Type::Int) {
        val.data.__int = pow(val.data.__int, other.data.__int);
    } else if (val.type == Type::Float && other.type == Type::Float) {
        val.data.__float = powf(val.data.__float, other.data.__float);
    } else val.type = Type::Null;
    return val;
}

llama::Value llama::Value::_eq(Value other) {
    Value val = Value(false);
    if (val.type != other.type) return val;

    if (val.type == Type::Bool) {
        val.data.__bool = data.__bool == other.data.__bool;
    } else if (val.type == Type::Int) {
        val.data.__bool = data.__int == other.data.__int;
    } else if (val.type == Type::Float) {
        val.data.__bool = data.__float == other.data.__float;
    }

    return val;
}

llama::Value llama::Value::_lt(Value other) {
    Value val = Value(false);
    if (val.type == Type::Bool && other.type == Type::Bool) {
        val.data.__bool = data.__bool < other.data.__bool;
    } else if (val.type == Type::Int && other.type == Type::Int) {
        val.data.__bool = data.__int < other.data.__int;
    } else if (val.type == Type::Float && other.type == Type::Float) {
        val.data.__bool = data.__float < other.data.__float;
    }
    return val;
}

llama::Value llama::Value::_le(Value other) {
    Value val = Value(false);
    if (val.type == Type::Bool && other.type == Type::Bool) {
        val.data.__bool = data.__bool <= other.data.__bool;
    } else if (val.type == Type::Int && other.type == Type::Int) {
        val.data.__bool = data.__int <= other.data.__int;
    } else if (val.type == Type::Float && other.type == Type::Float) {
        val.data.__bool = data.__float <= other.data.__float;
    }
    return val;
}

llama::Value llama::Value::_gt(Value other) {
    Value val = Value(false);
    if (val.type == Type::Bool && other.type == Type::Bool) {
        val.data.__bool = data.__bool > other.data.__bool;
    } else if (val.type == Type::Int && other.type == Type::Int) {
        val.data.__bool = data.__int > other.data.__int;
    } else if (val.type == Type::Float && other.type == Type::Float) {
        val.data.__bool = data.__float > other.data.__float;
    }
    return val;
}

llama::Value llama::Value::_ge(Value other) {
    Value val = Value(false);
    if (val.type == Type::Bool && other.type == Type::Bool) {
        val.data.__bool = data.__bool >= other.data.__bool;
    } else if (val.type == Type::Int && other.type == Type::Int) {
        val.data.__bool = data.__int >= other.data.__int;
    } else if (val.type == Type::Float && other.type == Type::Float) {
        val.data.__bool = data.__float >= other.data.__float;
    }
    return val;
}

llama::Value llama::Value::_ne(Value other) {
    Value val = Value(false);
    if (val.type == Type::Bool && other.type == Type::Bool) {
        val.data.__bool = data.__bool != other.data.__bool;
    } else if (val.type == Type::Int && other.type == Type::Int) {
        val.data.__bool = data.__int != other.data.__int;
    } else if (val.type == Type::Float && other.type == Type::Float) {
        val.data.__bool = data.__float != other.data.__float;
    }
    return val;
}

llama::Value llama::Value::_negate() {
    Value val = * this;
    switch (val.type) {
        case Type::Int: {
            val.data.__int = -val.data.__int;
            break;
        }
        case Type::Float: {
            val.data.__float = -val.data.__float;
            break;
        }
        default: val.type = Type::Null;
    }
    return val;
}

llama::Value llama::Value::_promote() {
    Value val = * this;
    switch (val.type) {
        case Type::Int: {
            val.data.__int = +val.data.__int;
            break;
        }
        case Type::Float: {
            val.data.__float = +val.data.__float;
            break;
        }
        default: val.type = Type::Null;
    }
    return val;
}

llama::Value llama::Value::_sizeof() {
    return Value((int)(size()));
}

/* -=- Data management -=- */
size_t llama::Value::size() {
    switch (type) {
        case Type::Null:     return 0;
        case Type::Bool:     return sizeof(bool);
        case Type::Int:      return sizeof(int);
        case Type::Float:    return sizeof(double);
        case Type::String:   return 0;
        case Type::List:     return 0;
        case Type::Object:   return 0;
        case Type::Userdata: return 0;
        case Type::Dynamic:  return 0;
        default:             return 0;
    }
}

/* -=- Converters -=- */
llama::Value llama::Value::convert(std::string conv_type) {
    Value v = Value();
    switch (type) {
        case Type::Bool: {
            if (conv_type == "int") {
                v.type         = Type::Float;
                v.data.__float = (int)data.__bool;
            } else if (conv_type == "float") {
                v.type         = Type::Float;
                v.data.__float = (double)data.__bool;
            }
            break;
        }
        case Type::Int: {
            if (conv_type == "float") {
                v.type         = Type::Float;
                v.data.__float = (double)data.__int;
            }
            break;
        }
        case Type::Float: {
            if (conv_type == "int") {
                v.type       = Type::Int;
                v.data.__int = (int)data.__float;
            }
            break;
        }
        default: break;
    }
    return v;
}

std::string llama::Value::as_string() {
    if (type == Type::Null) return "null";

    switch (type) {
        case Type::Bool:   return std::string(data.__bool ? "true" : "false");
        case Type::Int:    return std::to_string(data.__int);
        case Type::Float:  return std::to_string(data.__float);
        case Type::List: {
            // std::string str = "[";
            // for (size_t i = 0; i < data.__list->__size; ++i) {
            //     if (i > 0) str += ",";
            //     str += data.__list->__data[i].as_string();
            // }
            return "LIST TO STRING IS WIP!!!!!"; // TODO: can you fucking read the string?
        }
        default: return "unknown";
    }
}

/* -=- Utils -=- */
const char * llama::Value::type_str() {
    switch (type) {
        case Type::Null:     return "null";
        case Type::Bool:     return "bool";
        case Type::Int:      return "int";
        case Type::Float:    return "float";
        case Type::String:   return "string";
        case Type::List:     return "list";
        case Type::Object:   return "object"; // TODO: make this more specific to the object
        case Type::Userdata: return "userdata";
        case Type::Dynamic:  return "dynamic"; // TODO: revise the necessity of dynamic types
        default:             return "unknown";
    }
}