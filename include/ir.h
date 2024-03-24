#ifndef LLAMA_IR_H
#define LLAMA_IR_H

#include <bytecode.h>

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <stack>

#define MAKE_OP(__name, __cnt, __flag) InstInfo(GET_OP(__name), (#__name), (__cnt), (__flag))
#define DEF_OP(__name, __cnt, __flag)  { GET_OP(__name), MAKE_OP(__name, __cnt, __flag) }

namespace llama {
    class Module;

    class InstInfo {
    public:
        InstInfo();
        InstInfo(unsigned char m_opcode, const char * m_name, size_t m_size, unsigned short m_flags);
        ~InstInfo();

        unsigned char opcode;
        const char *  name;
        size_t        size;

        unsigned short flags;
    };

    class InstData {
    public:
        InstData(unsigned char m_opcode = 0x00, int32_t arg1 = 0, int32_t arg2 = 0, int32_t arg3 = 0);
        ~InstData();

        unsigned char opcode;
        int32_t       args[3];

        InstInfo get_info();

        std::string dump();
    };

    class IRBuilder {
    public:
        IRBuilder();
        IRBuilder(const IRBuilder & m_ir);
        ~IRBuilder();

        void _jp(int n);
        void _jz(int n);
        void _jnz(int n);
        void _block(int n);
        void _if(int n);
        void _else(int n);
        void _loop(int n);
        void _repeat();
        void _break();
        void _end(int n);
        void _pushnull();
        void _pushtrue();
        void _pushfalse();
        void _pushint(int v);
        void _pushfloat(double v);
        void _pushstring(int str);
        void _pushlist();
        void _pushobject(int class_name);
        void _pushdyn();
        void _pushfunc(int idx);
        void _setglobal(int name, int idx);
        void _getglobal(int name);
        void _setproperty(int name, int idx);
        void _getproperty(int name, int idx);
        void _setindex(int idx);
        void _getindex(int idx);
        void _newglobal(int name);
        void _newlocal(int name);
        void _pop();
        void _popn(int n);
        void _add();
        void _sub();
        void _mul();
        void _div();
        void _mod();
        void _pow();
        void _negate();
        void _promote();
        void _bitnot();
        void _bitand();
        void _bitor();
        void _bitxor();
        void _bitshl();
        void _bitshr();
        void _not();
        void _and();
        void _or();
        void _eq();
        void _lt();
        void _le();
        void _gt();
        void _ge();
        void _ne();
        void _sizeof();
        void _lenof();
        void _typeof();
        void _call(int argc);
        void _callv(int argc);
        void _return();
        void _returnv();
        void _ref();
        void _refglobal(int name);
        void _refproperty(int name);
        void _refindex(int idx);
        void _refset(int idx);
        void _breakpoint();
        void _typecheck(int type);

        void _pushstring(std::string str);
        void _pushobject(std::string class_name);
        void _setglobal(std::string name, int idx);
        void _getglobal(std::string name);
        void _setproperty(std::string name, int idx);
        void _getproperty(std::string name, int idx);
        void _newglobal(std::string name);
        void _newlocal(std::string name);
        void _refglobal(std::string name);
        void _refproperty(std::string name);
        void _typecheck(std::string type);

        void push_if();
        void push_else();
        void push_loop();
        void push_block();
        void end_block();

        size_t   size();
        size_t   real_size();
        size_t   inst_size(unsigned char opcode);
        void     push(InstData & inst);
        void     pop();
        void     erase(size_t idx); // NOTE: instructions depending on jump offsets DOESN'T change
        void     set(InstData inst, size_t idx);
        InstData at(size_t idx);

        void optimize();
        
        void        set_module(Module * m_mod);
        Module *    get_module();
        std::string disassemble();
        void        read(std::vector<unsigned char> & data);
        size_t      read_inst(std::vector<unsigned char> & data, size_t i);
        void        build(std::vector<unsigned char> & data);
        size_t      build_inst(std::vector<unsigned char> & data, size_t i);

        void dump();
    private:
        std::vector<InstData> ops;
        std::stack<size_t>    blocks;
        Module *              mod;
    };
}

#endif