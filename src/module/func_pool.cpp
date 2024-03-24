/* -=============
     Includes
   =============- */

#include <error.h>
#include <module.h>
#include <ir.h>
#include <util.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>

/* -========================
     FunctionEntry class
   ========================- */

/* -=- (Con/des)tructors -=- */
llama::FunctionEntry::FunctionEntry() {
    ext = nullptr;
}

llama::FunctionEntry::FunctionEntry(const FunctionEntry & entry) {
    name = entry.name;
    args = entry.args;
    data = entry.data;
    ext  = entry.ext;
    line = entry.line;
}

llama::FunctionEntry::~FunctionEntry() {}

/* -=- Base functions -=- */
void llama::FunctionEntry::set_name(std::string m_name) {
    name = m_name;
}

std::string llama::FunctionEntry::get_name() {
    return name;
}

std::vector<unsigned char> & llama::FunctionEntry::get_data() {
    return data;
}

int llama::FunctionEntry::get_line() {
    return line;
}

void llama::FunctionEntry::set_line(int m_line) {
    line = m_line;
}

void llama::FunctionEntry::push_arg(Argument arg) {
    args.push_back(arg);
}

llama::FunctionEntry::Argument llama::FunctionEntry::get_arg(size_t idx) {
    if (idx >= args.size()) return Argument();
    return args[idx];
}

size_t llama::FunctionEntry::get_argc() {
    return args.size();
}

/* -=======================
     FunctionPool class
   =======================- */

/* -=- Base functions -=- */
size_t llama::FunctionPool::add(FunctionEntry & entry) {
    entries.push_back(entry);
    return entries.size() - 1;
}

size_t llama::FunctionPool::get(std::string name) {
    auto it = std::find_if(entries.begin(), entries.end(), [&](FunctionEntry & other) {
        return other.get_name() == name;
    });
    if (it != entries.end()) return std::distance(entries.begin(), it);

    return ERROR_IDX_BIN;
}

bool llama::FunctionPool::has(std::string name) {
    auto it = std::find_if(entries.begin(), entries.end(), [&](FunctionEntry & other) {
        return other.get_name() == name;
    });

    return it != entries.end();
}

size_t llama::FunctionPool::size() {
    return entries.size();
}

llama::FunctionEntry * llama::FunctionPool::at(size_t idx) {
    if (idx >= entries.size()) return nullptr;
    return &entries[idx];
}

/* -=- Formatting -=- */
std::string llama::FunctionPool::dump(size_t idx, bool show_code) {
    auto & entry = entries[idx];

    std::string str;
    str += "fn";
    if (!entry.get_name().empty()) {
        str.push_back(' ');
        str += entry.get_name();
    }
    str += "(";
    for (size_t i = 0; i < entry.get_argc(); ++i) {
        if (i > 0) str += ", ";

        auto arg = entry.get_arg(i);
        str += arg.field;
        if (!arg.type.empty()) {
            str += ": ";
            str += arg.type;
        }
    }
    str += "):";
    if (show_code) {
        str += "\n";
        IRBuilder ir = IRBuilder();
        ir.set_module(mod);
        ir.read(entry.get_data());
        str += ir.disassemble().c_str();
    }
    return str;
}