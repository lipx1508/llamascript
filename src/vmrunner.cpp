/* -=============
     Includes
   =============- */

#include <error.h>
#include <vmrunner.h>
#include <vm.h>
#include <module.h>
#include <bytecode.h>
#include <ir.h>
#include <util.h>

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <stack>

/* -===================
     VMRunner class
   ===================- */

/* -=- (Con/des)tructors -=- */
llama::VMRunner::VMRunner(VM * m_vm) {
    vm = m_vm;
}

llama::VMRunner::~VMRunner() {}

/* -=- Bytecode execution -=- */
llama::Status llama::VMRunner::exec(size_t argc, bool pop) {
    Status s = Ok;

    auto * log = vm->log;

    auto & fn_val = vm->stack.back();
    if (fn_val.type != Type::Function) {
        RUNTIMEERROR("attempt to call a %s value", fn_val.type_str());
        return Failure;
    }

    auto * func = vm->module->get_functions()->at(fn_val.data.__idx);
    if (func == nullptr) {
        RUNTIMEERROR("the function index %zu do not exist", fn_val.data.__idx);
        return Failure;
    }

    // TODO: make the stack and pretty much everything sandboxed
    auto & stack = vm->stack;
    if (pop) stack.pop_back();

    auto * consts = vm->module->get_constants();
    
    size_t pc    = 0;
    size_t insts = 0;
    bool   ret   = false;

    std::stack<std::pair<size_t, size_t>> repeats;

    auto get_arg = [&](size_t n) -> int32_t {
        return unpack<int32_t>(func->get_data(), pc + 1 + sizeof(int32_t) * n);
    };

    auto get_size = [&](unsigned char op) {
        return sizeof(int32_t) * InstData(op).get_info().size + 1;
    };

    auto new_global = [&](size_t idx) {
        auto * c = consts->at(idx);
        if (c == nullptr) return Failure;

        vm->globals[unpack<char[]>(c->get_data())] = Value();

        return Ok;
    };

    auto do_inst = [&]() -> Status {
        ++insts;

        unsigned char op = func->get_data()[pc];
        printf("executing op %.2x (at %zu)\n", (int)op, (size_t)pc);
        
        if ((InstData(op).get_info().flags & GET_FLAG(STACKARG)) && stack.size() == 0) {
            PANIC("invalid stack access");
            return Failure;
        }

        switch (op) {
            case GET_OP(NOP): break;
            case GET_OP(JP): {
                pc += get_arg(0);
                break;
            }
            case GET_OP(JZ): {
                if (!(stack.back().data.__bool)) pc += get_arg(0);
                break;
            }
            case GET_OP(JNZ): {
                if (stack.size() > 0 && stack.back().data.__bool) pc += get_arg(0);
                break;
            }
            case GET_OP(BLOCK): {
                break;
            }
            case GET_OP(IF): {
                if (stack.back().data.__bool) {
                    size_t i = get_arg(0);
                    while (i-- > 0) {
                        pc += get_size(func->get_data()[pc]);
                    }
                }
                break;
            }
            case GET_OP(ELSE): {
                // TODO: brainstorm this one because I'm stupid
                break;
            }
            case GET_OP(LOOP): {
                size_t i = get_arg(0);
                while (i-- > 0) {
                    pc += get_size(func->get_data()[pc]);
                }
                break;
            }
            case GET_OP(REPEAT): {
                break;
            }
            case GET_OP(BREAK): {
                break;
            }
            case GET_OP(END): {
                
                break;
            }
            case GET_OP(PUSHNULL): {
                stack.push_back(Value());
                break;
            }
            case GET_OP(PUSHTRUE): {
                stack.push_back(Value(true));
                break;
            }
            case GET_OP(PUSHFALSE): {
                stack.push_back(Value(false));
                break;
            }
            case GET_OP(PUSHINT): {
                size_t idx = get_arg(0);

                auto * c = consts->at(idx);
                if (c == nullptr) {
                    PANIC("constant pool index %zu does not exist", idx);
                    return Failure;
                }
                if (c->get_type() != ConstantEntry::Type::Int) {
                    PANIC("constant index %zu is not an integer", idx);
                    return Failure;
                }

                int val = unpack<int32_t>(c->get_data());
                stack.push_back(Value(val));
                break;
            }
            case GET_OP(PUSHFLOAT): {
                size_t idx = get_arg(0);

                auto * c = consts->at(idx);
                if (c == nullptr) {
                    PANIC("constant pool index %zu does not exist", idx);
                    return Failure;
                }
                if (c->get_type() != ConstantEntry::Type::Float) {
                    PANIC("constant index %zu is not a float", idx);
                    return Failure;
                }

                stack.push_back(Value(unpack<double>(c->get_data())));
                break;
            }
            case GET_OP(PUSHSTRING): {
                break;
            }
            case GET_OP(PUSHLIST): {
                break;
            }
            case GET_OP(PUSHOBJECT): {
                break;
            }
            case GET_OP(PUSHDYN): {
                break;
            }
            case GET_OP(PUSHFUNC): {
                break;
            }
            case GET_OP(SETGLOBAL): {
                // TODO: implement this crap
                break;
            }
            case GET_OP(GETGLOBAL): {
                size_t idx = get_arg(0);
                auto * c   = consts->at(idx);
                if (c == nullptr) {
                    PANIC("constant pool index %zu does not exist", idx);
                    return Failure;
                }
                
                std::string name = unpack<char[]>(c->get_data());
                
                auto v = vm->globals[name];
                if (v.type == Type::Null) {
                    RUNTIMEERROR("the value \"%s\" was not declared in this scope", name.c_str());
                    return Failure;
                }

                stack.push_back(v);
                break;
            }
            case GET_OP(SETPROPERTY): {
                // TODO: implement this crap
                break;
            }
            case GET_OP(GETPROPERTY): {
                // TODO: implement this crap
                stack.push_back(Value());
                break;
            }
            case GET_OP(SETINDEX): {
                // TODO: implement this crap
                break;
            }
            case GET_OP(GETINDEX): {
                // TODO: implement this crap
                stack.push_back(Value());
                break;
            }
            case GET_OP(NEWGLOBAL): {
                s = new_global(get_arg(0));
                if (s == Failure) {
                    return Failure;
                }
                break;
            }
            case GET_OP(NEWLOCAL): {
                // TODO: implement this crap
                break;
            }
            case GET_OP(POP): {
                stack.pop_back();
                break;
            }
            case GET_OP(POPN): {
                vm->popn(get_arg(0));
                break;
            }
            case GET_OP(ADD): {
                Value & a = stack[stack.size() - 2];
                Value & b = stack[stack.size() - 1];

                Value v = a._add(b);
                if (v.type == Type::Null) {
                    RUNTIMEERROR("cannot add a value of type %s to a value of type %s", a.type_str(), b.type_str());
                    return Failure;
                }
                a = v;
                stack.pop_back();
                break;
            }
            case GET_OP(SUB): {
                Value & a = stack[stack.size() - 2];
                Value & b = stack[stack.size() - 1];

                Value v = a._sub(b);
                if (v.type == Type::Null) {
                    RUNTIMEERROR("cannot subtract a value of type %s to a value of type %s", a.type_str(), b.type_str());
                    return Failure;
                }
                a = v;
                stack.pop_back();
                break;
            }
            case GET_OP(MUL): {
                Value & a = stack[stack.size() - 2];
                Value & b = stack[stack.size() - 1];

                Value v = a._mul(b);
                if (v.type == Type::Null) {
                    RUNTIMEERROR("cannot multiply a value of type %s by a value of type %s", a.type_str(), b.type_str());
                    return Failure;
                }
                a = v;
                stack.pop_back();
                break;
            }
            case GET_OP(DIV): {
                Value & a = stack[stack.size() - 2];
                Value & b = stack[stack.size() - 1];

                Value v = a._div(b);
                if (v.type == Type::Null) {
                    RUNTIMEERROR("cannot divide a value of type %s by a value of type %s", a.type_str(), b.type_str());
                    return Failure;
                }
                a = v;
                stack.pop_back();
                break;
            }
            case GET_OP(MOD): {
                Value & a = stack[stack.size() - 2];
                Value & b = stack[stack.size() - 1];

                Value v = a._mod(b);
                if (v.type == Type::Null) {
                    RUNTIMEERROR("cannot obtain the remainder of a value of type %s by a value of type %s", a.type_str(), b.type_str());
                    return Failure;
                }
                a = v;
                stack.pop_back();
                break;
            }
            case GET_OP(POW): {
                Value & a = stack[stack.size() - 2];
                Value & b = stack[stack.size() - 1];

                Value v = a._pow(b);
                if (v.type == Type::Null) {
                    RUNTIMEERROR("cannot power a value of type %s by a value of type %s", a.type_str(), b.type_str());
                    return Failure;
                }
                a = v;
                stack.pop_back();
                break;
            }
            case GET_OP(NEGATE): {
                Value & a = stack[stack.size() - 2];

                Value v = a._negate();
                if (v.type == Type::Null) {
                    RUNTIMEERROR("cannot negate value of type %s", a.type_str());
                    return Failure;
                }

                a = v;
                break;
            }
            case GET_OP(PROMOTE): {
                Value & a = stack[stack.size() - 2];

                Value v = a._promote();
                if (v.type == Type::Null) {
                    RUNTIMEERROR("cannot negate value of type %s", a.type_str());
                    return Failure;
                }

                a = v;
                break;
            }
            case GET_OP(BITNOT): {
                break;
            }
            case GET_OP(BITAND): {
                break;
            }
            case GET_OP(BITOR): {
                break;
            }
            case GET_OP(BITXOR): {
                break;
            }
            case GET_OP(BITSHL): {
                break;
            }
            case GET_OP(BITSHR): {
                break;
            }
            case GET_OP(BITROL): {
                break;
            }
            case GET_OP(BITROR): {
                break;
            }
            case GET_OP(NOT): {
                Value & a = stack[stack.size() - 1];
                if (a.type == Type::Bool) {
                    a.data.__bool = !a.data.__bool;
                } else {
                    RUNTIMEERROR("cannot logical 'not' a %s value", a.type_str());
                    return Failure;
                }
                break;
            }
            case GET_OP(AND): {
                Value & a = stack[stack.size() - 2];
                Value & b = stack[stack.size() - 1];
                if (a.type == Type::Bool) {
                    a.data.__bool = a.data.__bool && b.data.__bool;
                } else {
                    RUNTIMEERROR("cannot logical 'and' a %s value", a.type_str());
                    return Failure;
                }
                stack.pop_back();
                break;
            }
            case GET_OP(OR): {
                Value & a = stack[stack.size() - 2];
                Value & b = stack[stack.size() - 1];
                if (a.type == Type::Bool) {
                    a.data.__bool = a.data.__bool || b.data.__bool;
                } else {
                    RUNTIMEERROR("cannot logical 'or' a %s value", a.type_str());
                    return Failure;
                }
                stack.pop_back();
                break;
            }
            case GET_OP(EQ): {
                Value & a = stack[stack.size() - 2];
                a = a._eq(stack[stack.size() - 1]);
                stack.pop_back();
                break;
            }
            case GET_OP(LT): {
                Value & a = stack[stack.size() - 2];
                a = a._lt(stack[stack.size() - 1]);
                stack.pop_back();
                break;
            }
            case GET_OP(LE): {
                Value & a = stack[stack.size() - 2];
                a = a._le(stack[stack.size() - 1]);
                stack.pop_back();
                break;
            }
            case GET_OP(GT): {
                Value & a = stack[stack.size() - 2];
                a = a._gt(stack[stack.size() - 1]);
                stack.pop_back();
                break;
            }
            case GET_OP(GE): {
                Value & a = stack[stack.size() - 2];
                a = a._ge(stack[stack.size() - 1]);
                stack.pop_back();
                break;
            }
            case GET_OP(NE): {
                Value & a = stack[stack.size() - 2];
                a = a._ne(stack[stack.size() - 1]);
                stack.pop_back();
                break;
            }
            case GET_OP(SIZEOF): {
                break;
            }
            case GET_OP(LENOF): {
                break;
            }
            case GET_OP(TYPEOF): {
                break;
            }
            case GET_OP(INSTANCEOF): {
                break;
            }
            case GET_OP(THIS): {
                break;
            }
            case GET_OP(AS): {
                size_t idx = get_arg(0);

                auto * c = consts->at(idx);
                if (c == nullptr) 
                    PANIC("constant pool index %zu does not exist", idx);

                const char * type_name = unpack<char[]>(c->get_data());

                Value & value = stack.back();
                if (std::string(value.type_str()) == type_name) break;

                Value conv = value.convert(type_name);
                if (conv.type != Type::Null) {
                    stack.pop_back();
                    stack.push_back(conv);
                } else {
                    RUNTIMEERROR("the type %s is not convertible to the type %s", value.type_str(), type_name);
                    return Failure;
                }
                break;
            }
            case GET_OP(CALL): {
                Status s = exec(get_arg(0), true);
                if (s == Failure) return Failure;
                break;
            }
            case GET_OP(CALLV): {
                // TODO: maybe remove this opcode
                break;
            }
            case GET_OP(RETURN): {
                ret = true;
                break;
            }
            case GET_OP(RETURNV): {
                ret = true;
                break;
            }
            case GET_OP(BREAKPOINT): {
                INFO("breakpoint called at %.8x", (uint32_t)(pc));
                break;
            }
            case GET_OP(REF): {
                break;
            }
            case GET_OP(REFGLOBAL): {
                break;
            }
            case GET_OP(REFPROPERTY): {
                break;
            }
            case GET_OP(REFINDEX): {
                break;
            }
            case GET_OP(REFSET): {
                break;
            }
            case GET_OP(TYPECHECK): {
                break;
            }
        }
        pc += get_size(op);

        return Ok;
    };

    while (pc < func->get_data().size() && !ret) {
        do_inst();
    }

    return s;
}