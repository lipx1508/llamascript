/* -=============
     Includes
   =============- */

#include <error.h>
#include <vm.h>
#include <vmrunner.h>
#include <lexer.h>
#include <analyser.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <ctime>
#include <string>

/* -=============
     VM class
   =============- */

/* -=- (Con/des)tructors -=- */
llama::VM::VM() {
    log    = new Logger();
    module = new Module();
}

llama::VM::VM(const VM & vm) {
    log    = new Logger();
    module = new Module(* vm.module);
}

llama::VM::~VM() {
    delete log;
    delete module;
}

/* -=- Stack management -=- */
void llama::VM::push() {
    stack.push_back(Value());
}

void llama::VM::push(Value & v) {
    stack.push_back(v);
}

void llama::VM::push(Value && v) {
    stack.push_back(v);
}

void llama::VM::set_global(std::string name, int value) {

}

void llama::VM::get_global(std::string name) {

}

void llama::VM::set_property(std::string name, int idx, int value) {
    // TODO: implement this
}

void llama::VM::get_property(std::string name, int idx) {
    // TODO: implement this
}

void llama::VM::set_index(int idx) {
    // Checkes for the bounds of the index
    Value * list  = get(idx);
    Value * index = get(-2);
    Value * val   = get(-1);
    
    if (list == nullptr || val == nullptr || index == nullptr) {
        RUNTIMEERROR("attempted to index a null value");
        return;
    }

    if (list->type != Type::List) {
        RUNTIMEERROR("attempted to index a %s value", list->type_str());
        return;
    } else if (index->type != Type::Int) {
        RUNTIMEERROR("index must be an integer, got %s instead", index->type_str());
        return;
    }

    //list->data.__list->set(index->data.__int, * val);

    popn(2);
}

void llama::VM::get_index(int idx) {
    // TODO: implement this
}

void llama::VM::new_global(std::string name) {
    if (globals.find(name) != globals.end()) {
        RUNTIMEERROR("the global %s was already declared in this scope", name.c_str());
        return;
    }
    globals.insert({ name, Value() });
}

void llama::VM::new_local(std::string name) {
    // TODO: implement this

}

void llama::VM::pop() {
    stack.pop_back();
}

void llama::VM::popn(int n) {
    if (n == 0) return;
    stack.erase(stack.end() - n, stack.end());
}

/* -=- Function calls -=- */
llama::Status llama::VM::call(size_t argc, bool pop) {
    VMRunner runner = VMRunner(this);
    return runner.exec(argc, pop);
}

llama::Status llama::VM::callv(size_t argc, bool pop) {
    VMRunner runner = VMRunner(this);
    return runner.exec(argc, pop);
}

/* -=- Utilities -=- */
llama::Value * llama::VM::get(int idx) {
    size_t i = REAL_IDX(idx);
    if (i > stack.size()) return nullptr;
    return &stack[i];
}

void llama::VM::dump() {
    printf("-- STACK DUMP --\n");
    for (size_t i = 0; i < stack.size(); ++i) {
        printf("%zu: %s (%s)\n", i, stack[i].as_string().c_str(), stack[i].type_str());
    }

    printf("-- GLOBALS DUMP --\n");
    for (auto & i : globals) {
        printf("%s: %s (%s)\n", i.first.c_str(), i.second.as_string().c_str(), i.second.type_str());
    }
}

/* -=- Code loading -=- */
llama::Status llama::VM::load_string(const char * str) {
    log->set_source("string");
    Status s = read(str);
    log->reset();
    return s;
}

llama::Status llama::VM::load_file(const char * path) {
    FILE * f = fopen(path, "r");
    if (f == nullptr) {
        RUNTIMEERROR(strerror(ferror(f)));
        return Failure;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    std::string str;
    str.resize(size);

    fread(&str[0], sizeof(char), size, f);

    fclose(f);

    log->set_source(path);
    Status s = read(str);
    log->reset();

    return s;
}

llama::Status llama::VM::do_string(const char * str) {
    Status s = load_string(str);
    if (s != Failure) {
        Value fn = Value(module->get_functions()->size() - 1, Type::Function);
        stack.push_back(fn);
        call(0, true);
    }
    return s;
}

llama::Status llama::VM::do_file(const char * path) {
    Status s = load_file(path);
    if (s != Failure) {
        Value fn = Value(module->get_functions()->size() - 1, Type::Function);
        stack.push_back(fn);
        call(0, true);
    }
    return s;
}

llama::Status llama::VM::read(std::string str) {
    Status status = Ok;

#ifdef LLAMA_DEBUG
    clock_t start = clock();
#endif

    Lexer lex;
    lex.parse(log, str);
    //lex.dump();

    Analyser analysis;
    analysis.read(module, log, &lex);
    //analysis.dump();

    module->dump();

#ifdef LLAMA_DEBUG
    double secs   = (double)(clock() - start) / CLOCKS_PER_SEC;
    double millis = secs * 1000;
    INFO("finished parsing in %fms (%fs)", millis, secs);
#endif

    return status;
}