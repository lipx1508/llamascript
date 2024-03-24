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
     ConstantEntry class
   ========================- */

/* -=- (Con/des)tructors -=- */
llama::ConstantEntry::ConstantEntry() {
    type = Type::None;
}

llama::ConstantEntry::ConstantEntry(const ConstantEntry & entry) {
    type = entry.type;
    data = entry.data;
}

llama::ConstantEntry::~ConstantEntry() {
    type = Type::None;
}

llama::ConstantEntry::ConstantEntry(int v) {
    * this = ConstantEntry(&v, sizeof(v), Type::Int);
}

llama::ConstantEntry::ConstantEntry(double v) {
    * this = ConstantEntry(&v, sizeof(v), Type::Float);
}

llama::ConstantEntry::ConstantEntry(std::string str) {
    type = Type::String;
    data.resize(str.length() + 1);
    memcpy(data.data(), str.data(), str.length());
    data.back() = '\0';
}

llama::ConstantEntry::ConstantEntry(const void * ptr, size_t size, Type m_type) {
    type = m_type;
    data.resize(size);
    memcpy(data.data(), ptr, size);
}

/* -=- Converters -=- */
int llama::ConstantEntry::as_int() {
    if (type != Type::Int) return 0;
    return unpack<int>(data);
}

double llama::ConstantEntry::as_float() {
    if (type != Type::Float) return 0;
    return unpack<double>(data);
}

std::string llama::ConstantEntry::as_string() {
    if (type != Type::String) return std::string();
    return std::string(unpack<char[]>(data));
}

/* -=- (S/g)etters -=- */
llama::ConstantEntry::Type llama::ConstantEntry::get_type() {
    return type;
}

std::vector<unsigned char> & llama::ConstantEntry::get_data() {
    return data;
}

/* -=- Operator overloads -=- */
bool llama::ConstantEntry::operator==(const ConstantEntry & other) {
    if (data.size() != other.data.size() || type != other.type) return false;
    return memcmp(data.data(), other.data.data(), data.size()) == 0;
}

/* -=- Formatters -=- */
std::string llama::ConstantEntry::dump() {
    switch (type) {
        case Type::Userdata: {
            std::string str;
            const char hex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
            
            for (auto & byte : data) {
                str += hex[(byte & 0xf0) >> 4];
                str += hex[(byte & 0x0f) >> 0];
            }

            return str;
        }
        case Type::Int: {
            return std::to_string(as_int());
        }
        case Type::Float: {
            return std::to_string(as_float());
        }
        case ConstantEntry::Type::String: {
            return as_string();
        }
        default: return "unknown";
    }
}

/* -=======================
     ConstantPool class
   =======================- */

/* -=- Base functions -=- */
size_t llama::ConstantPool::get(ConstantEntry entry) {
    auto it = std::find_if(entries.begin(), entries.end(), [&](ConstantEntry & other) {
        return entry == other;
    });

    if (it != entries.end()) return std::distance(entries.begin(), it);

    entries.push_back(entry);
    return entries.size() - 1;
}

llama::ConstantEntry * llama::ConstantPool::at(size_t idx) {
    if (idx >= entries.size()) return nullptr;
    return &entries[idx];
}

size_t llama::ConstantPool::size() {
    return entries.size();
}

/* -=- Formatters -=- */
std::string llama::ConstantPool::dump() {
    std::string str;
    for (size_t i = 0; i < entries.size(); ++i) {
        str += std::to_string(i);
        str += ": ";
        str += entries[i].dump();
        str += "\n";
    }
    return str;
}