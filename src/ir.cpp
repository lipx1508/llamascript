/* -=============
     Includes
   =============- */

#include <ir.h>
#include <bytecode.h>
#include <module.h>
#include <util.h>

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>
#include <stack>
#include <map>

/* -===============
     Internals
   ================- */

namespace llama {
    static std::map<unsigned char, InstInfo> insts = {
        DEF_OP(NOP,         0, 0), 
        DEF_OP(JP,          1, 0), 
        DEF_OP(JZ,          1, 0), 
        DEF_OP(JNZ,         1, 0), 
        DEF_OP(BLOCK,       1, GET_FLAG(IMMUTARG) | GET_FLAG(ISBLOCK)), 
        DEF_OP(IF,          1, GET_FLAG(IMMUTARG) | GET_FLAG(ISBLOCK)), 
        DEF_OP(ELSE,        1, GET_FLAG(IMMUTARG) | GET_FLAG(ISBLOCK) | GET_FLAG(ISEND)), 
        DEF_OP(LOOP,        1, GET_FLAG(IMMUTARG) | GET_FLAG(ISBLOCK)), 
        DEF_OP(REPEAT,      0, 0), 
        DEF_OP(BREAK,       0, GET_FLAG(ISEND)), 
        DEF_OP(END,         1, GET_FLAG(IMMUTARG) | GET_FLAG(ISEND)), 
        DEF_OP(PUSHNULL,    0, 0), 
        DEF_OP(PUSHTRUE,    0, 0), 
        DEF_OP(PUSHFALSE,   0, 0), 
        DEF_OP(PUSHINT,     1, GET_FLAG(CONSTARG)), 
        DEF_OP(PUSHFLOAT,   1, GET_FLAG(CONSTARG)), 
        DEF_OP(PUSHSTRING,  1, GET_FLAG(CONSTARG)), 
        DEF_OP(PUSHLIST,    0, 0), 
        DEF_OP(PUSHOBJECT,  1, GET_FLAG(CONSTARG)), 
        DEF_OP(PUSHDYN,     0, 0), 
        DEF_OP(PUSHFUNC,    1, GET_FLAG(CONSTARG)), 
        DEF_OP(SETGLOBAL,   2, GET_FLAG(CONSTARG) | GET_FLAG(STACKARG)), 
        DEF_OP(GETGLOBAL,   1, GET_FLAG(CONSTARG)), 
        DEF_OP(SETPROPERTY, 2, GET_FLAG(STACKARG) | GET_FLAG(CONSTARG)), 
        DEF_OP(GETPROPERTY, 2, GET_FLAG(STACKARG) | GET_FLAG(CONSTARG)), 
        DEF_OP(SETINDEX,    1, GET_FLAG(STACKARG)), 
        DEF_OP(GETINDEX,    1, GET_FLAG(STACKARG)), 
        DEF_OP(NEWGLOBAL,   1, GET_FLAG(CONSTARG)), 
        DEF_OP(NEWLOCAL,    1, GET_FLAG(CONSTARG)), 
        DEF_OP(POP,         0, 0), 
        DEF_OP(POPN,        1, GET_FLAG(IMMUTARG)), 
        DEF_OP(ADD,         0, GET_FLAG(STACKARG)), 
        DEF_OP(SUB,         0, GET_FLAG(STACKARG)), 
        DEF_OP(MUL,         0, GET_FLAG(STACKARG)), 
        DEF_OP(DIV,         0, GET_FLAG(STACKARG)), 
        DEF_OP(MOD,         0, GET_FLAG(STACKARG)), 
        DEF_OP(POW,         0, GET_FLAG(STACKARG)), 
        DEF_OP(NEGATE,      0, GET_FLAG(STACKARG)), 
        DEF_OP(PROMOTE,     0, GET_FLAG(STACKARG)), 
        DEF_OP(BITNOT,      0, GET_FLAG(STACKARG)), 
        DEF_OP(BITAND,      0, GET_FLAG(STACKARG)), 
        DEF_OP(BITOR,       0, GET_FLAG(STACKARG)), 
        DEF_OP(BITXOR,      0, GET_FLAG(STACKARG)), 
        DEF_OP(BITSHL,      0, GET_FLAG(STACKARG)), 
        DEF_OP(BITSHR,      0, GET_FLAG(STACKARG)), 
        DEF_OP(BITROL,      0, GET_FLAG(STACKARG)), 
        DEF_OP(BITROR,      0, GET_FLAG(STACKARG)), 
        DEF_OP(NOT,         0, GET_FLAG(STACKARG)), 
        DEF_OP(AND,         0, GET_FLAG(STACKARG)), 
        DEF_OP(OR,          0, GET_FLAG(STACKARG)), 
        DEF_OP(EQ,          0, GET_FLAG(STACKARG)), 
        DEF_OP(LT,          0, GET_FLAG(STACKARG)), 
        DEF_OP(LE,          0, GET_FLAG(STACKARG)), 
        DEF_OP(GT,          0, GET_FLAG(STACKARG)), 
        DEF_OP(GE,          0, GET_FLAG(STACKARG)), 
        DEF_OP(NE,          0, GET_FLAG(STACKARG)), 
        DEF_OP(SIZEOF,      0, GET_FLAG(STACKARG)), 
        DEF_OP(LENOF,       0, GET_FLAG(STACKARG)), 
        DEF_OP(TYPEOF,      0, GET_FLAG(STACKARG)), 
        DEF_OP(INSTANCEOF,  0, GET_FLAG(STACKARG)), 
        DEF_OP(THIS,        0, 0), 
        DEF_OP(AS,          0, GET_FLAG(STACKARG)), 
        DEF_OP(CALL,        1, GET_FLAG(IMMUTARG) | GET_FLAG(STACKARG)), 
        DEF_OP(CALLV,       1, GET_FLAG(IMMUTARG) | GET_FLAG(STACKARG)), 
        DEF_OP(RETURN,      0, GET_FLAG(STACKARG)), 
        DEF_OP(RETURNV,     0, 0), 
        DEF_OP(BREAKPOINT,  0, 0), 
        DEF_OP(REF,         0, 0), 
        DEF_OP(REFGLOBAL,   1, GET_FLAG(CONSTARG)), 
        DEF_OP(REFPROPERTY, 1, GET_FLAG(CONSTARG)), 
        DEF_OP(REFINDEX,    1, GET_FLAG(IMMUTARG)), 
        DEF_OP(REFSET,      1, GET_FLAG(IMMUTARG)), 
        DEF_OP(TYPECHECK,   1, GET_FLAG(CONSTARG) | GET_FLAG(STACKARG)), 
    };
}

/* -===================
     InstInfo class
   ===================- */

/* -=- (Con/des)tructors -=- */
llama::InstInfo::InstInfo() {
    opcode = 0xff;
    name   = "UNKNOWN";
    size   = 0;
}

llama::InstInfo::InstInfo(unsigned char m_opcode, const char * m_name, size_t m_size, unsigned short m_flags) {
    opcode = m_opcode;
    name   = m_name;
    size   = m_size;
    flags  = m_flags;
}

llama::InstInfo::~InstInfo() {}

/* -===================
     InstInfo class
   ===================- */

/* -=- (Con/des)tructors -=- */
llama::InstData::InstData(unsigned char m_opcode, int32_t arg1, int32_t arg2, int32_t arg3) {
    opcode  = m_opcode;
    args[0] = arg1;
    args[1] = arg2;
    args[2] = arg3;
}

llama::InstData::~InstData() {}

/* -=- (S/g)etters -=- */
llama::InstInfo llama::InstData::get_info() {
    return insts[opcode];
}

/* -=- Formatters -=- */
std::string llama::InstData::dump() {
    InstInfo info = get_info();

    std::string str = info.name;
    for (size_t i = 0; i < info.size; ++i) {
        str.push_back(' ');
        str += std::to_string(args[i]);
    }

    return str;
}

/* -====================
     IRBuilder class
   ====================- */

/* -=- (Con/des)tructors -=- */
llama::IRBuilder::IRBuilder() {}

llama::IRBuilder::IRBuilder(const IRBuilder & m_ir) {
    mod    = m_ir.mod;
    ops    = m_ir.ops;
    blocks = m_ir.blocks;
}

llama::IRBuilder::~IRBuilder() {}

/* -=- Base functions -=- */
void llama::IRBuilder::dump() {
    printf("-- ASMDUMP (size = %zu, %zu bytes) --\n%s\n", ops.size(), real_size(), disassemble().c_str());
}

/* -=- Instructions -=- */
void llama::IRBuilder::_jp(int n) {
    ops.push_back(InstData(GET_OP(JP), n));
}

void llama::IRBuilder::_jz(int n) {
    ops.push_back(InstData(GET_OP(JZ), n));
}

void llama::IRBuilder::_jnz(int n) {
    ops.push_back(InstData(GET_OP(JNZ), n));
}

void llama::IRBuilder::_block(int n) {
    ops.push_back(InstData(GET_OP(BLOCK), n));
}

void llama::IRBuilder::_if(int n) {
    ops.push_back(InstData(GET_OP(IF), n));
}

void llama::IRBuilder::_else(int n) {
    ops.push_back(InstData(GET_OP(ELSE), n));
}

void llama::IRBuilder::_loop(int n) {
    ops.push_back(InstData(GET_OP(LOOP), n));
}

void llama::IRBuilder::_end(int n) {
    ops.push_back(InstData(GET_OP(END), n));
}

void llama::IRBuilder::_repeat() {
    ops.push_back(InstData(GET_OP(REPEAT)));
}

void llama::IRBuilder::_break() {
    ops.push_back(InstData(GET_OP(BREAK)));
}

void llama::IRBuilder::_pushnull() {
    ops.push_back(InstData(GET_OP(PUSHNULL)));
}

void llama::IRBuilder::_pushtrue() {
    ops.push_back(InstData(GET_OP(PUSHTRUE)));
}

void llama::IRBuilder::_pushfalse() {
    ops.push_back(InstData(GET_OP(PUSHFALSE)));
}

void llama::IRBuilder::_pushint(int v) {
    ops.push_back(InstData(GET_OP(PUSHINT), mod->get_constants()->get(v)));
}

void llama::IRBuilder::_pushfloat(double v) {
    ops.push_back(InstData(GET_OP(PUSHFLOAT), mod->get_constants()->get(v)));
}

void llama::IRBuilder::_pushstring(int str) {
    ops.push_back(InstData(GET_OP(PUSHSTRING), str));
}

void llama::IRBuilder::_pushlist() {
    ops.push_back(InstData(GET_OP(PUSHLIST)));
}

void llama::IRBuilder::_pushobject(int class_name) {
    ops.push_back(InstData(GET_OP(PUSHOBJECT), class_name));
}

void llama::IRBuilder::_pushdyn() {
    ops.push_back(InstData(GET_OP(PUSHDYN)));
}

void llama::IRBuilder::_pushfunc(int idx) {
    ops.push_back(InstData(GET_OP(PUSHFUNC), idx));
}

void llama::IRBuilder::_setglobal(int name, int idx) {
    ops.push_back(InstData(GET_OP(SETGLOBAL), name, idx));
}

void llama::IRBuilder::_getglobal(int name) {
    ops.push_back(InstData(GET_OP(GETGLOBAL), name));
}

void llama::IRBuilder::_setproperty(int name, int idx) {
    ops.push_back(InstData(GET_OP(SETPROPERTY), name, idx));
}

void llama::IRBuilder::_getproperty(int name, int idx) {
    ops.push_back(InstData(GET_OP(GETPROPERTY), name, idx));
}

void llama::IRBuilder::_setindex(int idx) {
    ops.push_back(InstData(GET_OP(SETINDEX), idx));
}

void llama::IRBuilder::_getindex(int idx) {
    ops.push_back(InstData(GET_OP(GETINDEX), idx));
}

void llama::IRBuilder::_newglobal(int name) {
    ops.push_back(InstData(GET_OP(NEWGLOBAL), name));
}

void llama::IRBuilder::_newlocal(int name) {
    ops.push_back(InstData(GET_OP(NEWLOCAL), name));
}

void llama::IRBuilder::_pop() {
    ops.push_back(InstData(GET_OP(POP)));
}

void llama::IRBuilder::_popn(int n) {
    ops.push_back(InstData(GET_OP(POPN), n));
}

void llama::IRBuilder::_add() {
    ops.push_back(InstData(GET_OP(ADD)));
}

void llama::IRBuilder::_sub() {
    ops.push_back(InstData(GET_OP(SUB)));
}

void llama::IRBuilder::_mul() {
    ops.push_back(InstData(GET_OP(MUL)));
}

void llama::IRBuilder::_div() {
    ops.push_back(InstData(GET_OP(DIV)));
}

void llama::IRBuilder::_mod() {
    ops.push_back(InstData(GET_OP(MOD)));
}

void llama::IRBuilder::_pow() {
    ops.push_back(InstData(GET_OP(POW)));
}

void llama::IRBuilder::_negate() {
    ops.push_back(InstData(GET_OP(NEGATE)));
}

void llama::IRBuilder::_promote() {
    ops.push_back(InstData(GET_OP(PROMOTE)));
}

void llama::IRBuilder::_bitnot() {
    ops.push_back(InstData(GET_OP(BITNOT)));
}

void llama::IRBuilder::_bitand() {
    ops.push_back(InstData(GET_OP(BITAND)));
}

void llama::IRBuilder::_bitor() {
    ops.push_back(InstData(GET_OP(BITOR)));
}

void llama::IRBuilder::_bitxor() {
    ops.push_back(InstData(GET_OP(BITXOR)));
}

void llama::IRBuilder::_bitshl() {
    ops.push_back(InstData(GET_OP(BITSHL)));
}

void llama::IRBuilder::_bitshr() {
    ops.push_back(InstData(GET_OP(BITSHR)));
}

void llama::IRBuilder::_not() {
    ops.push_back(InstData(GET_OP(NOT)));
}

void llama::IRBuilder::_and() {
    ops.push_back(InstData(GET_OP(AND)));
}

void llama::IRBuilder::_or() {
    ops.push_back(InstData(GET_OP(OR)));
}

void llama::IRBuilder::_eq() {
    ops.push_back(InstData(GET_OP(EQ)));
}

void llama::IRBuilder::_lt() {
    ops.push_back(InstData(GET_OP(LT)));
}

void llama::IRBuilder::_le() {
    ops.push_back(InstData(GET_OP(LE)));
}

void llama::IRBuilder::_gt() {
    ops.push_back(InstData(GET_OP(GT)));
}

void llama::IRBuilder::_ge() {
    ops.push_back(InstData(GET_OP(GE)));
}

void llama::IRBuilder::_ne() {
    ops.push_back(InstData(GET_OP(NE)));
}

void llama::IRBuilder::_call(int argc) {
    ops.push_back(InstData(GET_OP(CALL), argc));
}

void llama::IRBuilder::_callv(int argc) {
    ops.push_back(InstData(GET_OP(CALLV), argc));
}

void llama::IRBuilder::_return() {
    ops.push_back(InstData(GET_OP(RETURN)));
}

void llama::IRBuilder::_returnv() {
    ops.push_back(InstData(GET_OP(RETURNV)));
}

void llama::IRBuilder::_ref() {
    ops.push_back(InstData(GET_OP(REF)));
}

void llama::IRBuilder::_refglobal(int name) {
    ops.push_back(InstData(GET_OP(REFGLOBAL), name));
}

void llama::IRBuilder::_refproperty(int name) {
    ops.push_back(InstData(GET_OP(REFPROPERTY), name));
}

void llama::IRBuilder::_refindex(int idx) {
    ops.push_back(InstData(GET_OP(REFINDEX), idx));
}

void llama::IRBuilder::_refset(int idx) {
    ops.push_back(InstData(GET_OP(REFSET), idx));
}

void llama::IRBuilder::_breakpoint() {
    ops.push_back(InstData(GET_OP(BREAKPOINT)));
}

void llama::IRBuilder::_typecheck(int type) {
    ops.push_back(InstData(GET_OP(TYPECHECK), type));
}

/* -=- Shortcut instructions -=- */
void llama::IRBuilder::_pushstring(std::string str) {
    _pushstring(mod->get_constants()->get(str));
}

void llama::IRBuilder::_pushobject(std::string class_name) {
    _pushobject(mod->get_constants()->get(class_name));
}

void llama::IRBuilder::_setglobal(std::string name, int idx) {
    _setglobal(mod->get_constants()->get(name), idx);
}

void llama::IRBuilder::_getglobal(std::string name) {
    _getglobal(mod->get_constants()->get(name));
}

void llama::IRBuilder::_setproperty(std::string name, int idx) {
    _setproperty(mod->get_constants()->get(name), idx);
}

void llama::IRBuilder::_getproperty(std::string name, int idx) {
    _getproperty(mod->get_constants()->get(name), idx);
}

void llama::IRBuilder::_newglobal(std::string name) {
    _newglobal(mod->get_constants()->get(name));
}

void llama::IRBuilder::_newlocal(std::string name) {
    _newlocal(mod->get_constants()->get(name));
}

void llama::IRBuilder::_refglobal(std::string name) {
    _refglobal(mod->get_constants()->get(name));
}

void llama::IRBuilder::_refproperty(std::string name) {
    _refproperty(mod->get_constants()->get(name));
}

void llama::IRBuilder::_typecheck(std::string type) {
    _typecheck(mod->get_constants()->get(type));
}

/* -=- Conditional statements handling -=- */
void llama::IRBuilder::push_if() {
    _if(0);
    blocks.push(size());
}

void llama::IRBuilder::push_else() {
    _else(0);
    ops.back().args[0] = size() - blocks.top();

    blocks.pop();
    blocks.push(size());
}

void llama::IRBuilder::push_loop() {
    _loop(0);
    blocks.push(size());
}

void llama::IRBuilder::push_block() {
    _block(0);
    blocks.push(size());
}

void llama::IRBuilder::end_block() {
    int32_t offset = size() - blocks.top();

    ops.back().args[0] = offset;

    _end(-offset);

    blocks.pop();
}

/* -=- Instruction management -=- */
size_t llama::IRBuilder::size() {
    return ops.size();
}

size_t llama::IRBuilder::real_size() {
    size_t size = 0;
    for (auto & op : ops) {
        size += inst_size(op.opcode);
    }
    return size;
}

size_t llama::IRBuilder::inst_size(unsigned char opcode) {
    return insts[opcode].size * sizeof(int32_t) + 1;
}

void llama::IRBuilder::push(InstData & inst) {
    ops.push_back(inst);
}

void llama::IRBuilder::pop() {
    ops.pop_back();
}

void llama::IRBuilder::erase(size_t idx) {
    ops.erase(ops.begin() + idx);
}

void llama::IRBuilder::set(InstData inst, size_t idx) {
    ops[idx] = inst;
}

llama::InstData llama::IRBuilder::at(size_t idx) {
    return ops[idx];
}

/* -=- Optimization and caching -=- */
void llama::IRBuilder::optimize() {
    
}

/* -=- Assembler and disassembler -=- */
void llama::IRBuilder::set_module(Module * m_mod) {
    mod = m_mod;
}

llama::Module * llama::IRBuilder::get_module() {
    return mod;
}

std::string llama::IRBuilder::disassemble() {
    std::string dis;

    size_t iden = 0;

    for (size_t i = 0; i < ops.size(); ++i) {
        unsigned char opcode = ops[i].opcode;
        
        dis += std::to_string(i) + " | ";

        if (opcode == GET_OP(ELSE) || opcode == GET_OP(END)) --iden;
        for (size_t j = 0; j <= iden; ++j) dis += "\t";
        if (opcode == GET_OP(ELSE) || (opcode >= GET_OP(BLOCK) && opcode <= GET_OP(LOOP))) ++iden;

        dis += ops[i].dump();

        auto info = ops[i].get_info();
        if (info.flags & GET_FLAG(CONSTARG)) {
            dis += " (";
            for (size_t j = 0; j < info.size; ++j) {
                if (j > 0) dis += ", ";
                auto * c = mod->get_constants()->at(ops[i].args[j]);
                if (c == nullptr) dis += std::to_string(ops[i].args[j]);
                else              dis += c->dump();
            }
            dis += ")";
        }

        dis.push_back('\n');
    }

    return dis;
}

void llama::IRBuilder::read(std::vector<unsigned char> & data) {
    ops.clear();

    size_t i = 0;
    while (i < data.size()) {
        i = read_inst(data, i);
    }
}

size_t llama::IRBuilder::read_inst(std::vector<unsigned char> & data, size_t i) {
    InstData op = InstData(data[i++]);

    size_t argc = op.get_info().size;
    for (size_t j = 0; j < argc; ++j) {
        op.args[j] = unpack<int32_t>(data, i);
        i += sizeof(int32_t);
    }

    ops.push_back(op);

    return i;
}

void llama::IRBuilder::build(std::vector<unsigned char> & data) {
    data.clear();
    data.reserve(real_size());

    size_t i = 0;
    while (i < ops.size()) {
        i = build_inst(data, i);
    }
}

size_t llama::IRBuilder::build_inst(std::vector<unsigned char> & data, size_t i) {
    data.push_back(ops[i].opcode);
    
    size_t inst_size = ops[i].get_info().size;
    for (size_t j = 0; j < inst_size; ++j) {
        pack<int32_t>(data, ops[i].args[j]);
    }

    return i + 1;
}