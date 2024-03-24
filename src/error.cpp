/* -=============
     Includes
   =============- */

#include <error.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <string>

/* -=====================
     LogSnippet class
   =====================- */

/* -=- (Con/des)tructors -=- */
llama::LogSnippet::LogSnippet() {
    start   = 0;
    end     = 0;
    line    = 0;
    collumn = 0;
}

llama::LogSnippet::LogSnippet(std::string str, size_t pos) {
    // I don't even know what I have written here but basically this just obtains the
    // line and collumn based on the index of the character in a string
    start   = pos;
    end     = str.length();
    line    = 1;
    collumn = 1;

    size_t i = 0;
    while (i < str.length()) {
        if (i < start) {
            if (str[i] == '\n') {
                ++line;
                collumn = 1;
                start   = i;
            } else {
                ++collumn;
            }
        } else if (str[i] == '\n') {
            end = i;
            break;
        }
        ++i;
    }

    if (i >= str.length()) {
        end     = str.length();
        collumn = 0;
    }
}

llama::LogSnippet::~LogSnippet() {}

/* -=================
     Logger class
   =================- */

/* -=- (Con/des)tructors -=- */
llama::Logger::Logger() {
    reset();
}

llama::Logger::~Logger() {}

/* -=- Base functions -=- */
void llama::Logger::reset() {
    source   = nullptr;
    __d_file = nullptr;
    __d_func = nullptr;
    __d_line = 0;

    err = "unknown log information";

    recoverable = false;
}

void llama::Logger::set_source(const char * file) {
    source = file;
}

void llama::Logger::set_snippet(LogSnippet m_snippet) {
    snippet = m_snippet;
}

void llama::Logger::set_debug(const char * file, int line, const char * function) {
    __d_file = file;
    __d_func = function;
    __d_line = line;
}

void llama::Logger::set_recoverable() {
    recoverable = true;
}

void llama::Logger::log(Type type, const char * fmt, ...) {
    err.clear();

    va_list args1;
    va_start(args1, fmt);
    size_t size = vsnprintf(nullptr, 0, fmt, args1) + 1;
    va_end(args1);

    err.resize(size);

    va_list args2;
    va_start(args2, fmt);
    vsnprintf(&err[0], err.size(), fmt, args2);
    va_end(args2);

    FILE * f = (type == Type::Info ? stdout : stderr);

    if (source != nullptr) fprintf(f, "%s:", source);

    if (snippet.line > 0) {
        fprintf(f, "%zu:", snippet.line);
        if (snippet.collumn > 0) {
            fprintf(f, "%zu: ", snippet.collumn);
        } else fputc(' ', f);
    } else if (source != nullptr) fputc(' ', f);

    const char * error = err.c_str();

    switch (type) {
        case Type::Info:         { fprintf(f, "info: %s\n", error);          break; }
        case Type::Warning:      { fprintf(f, "warning: %s\n", error);       break; }
        case Type::SyntaxError:  { fprintf(f, "syntax error: %s\n", error);  break; }
        case Type::RuntimeError: { fprintf(f, "runtime error: %s\n", error); break; }
        case Type::TypeError:    { fprintf(f, "type error: %s\n", error);    break; }
        case Type::Panic:        { fprintf(f, "PANIC! %s\n", error);         break; }
    }

#ifdef LLAMA_DEBUG
    if (__d_file != nullptr && __d_line > 0 && __d_func != nullptr) {
        fprintf(f, "\t- from %s:%d\n\t- in %s\n", __d_file, __d_line, __d_func);
    }
#endif

    if (type == Type::Panic) abort();

    // TODO: not abort in recoverable errors, looking out for more (like other normal and sane compilers)
    if (type >= Type::SyntaxError && !recoverable) exit(-1);
}