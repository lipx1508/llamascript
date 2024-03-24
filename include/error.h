#ifndef LLAMA_ERROR_H
#define LLAMA_ERROR_H

#include <climits>
#include <string>

#define ERROR_IDX     (SIZE_MAX)
#define ERROR_IDX_BIN (UINT32_MAX)

#define LOG(type, ...) {\
            log->set_debug(__FILE__, __LINE__, __PRETTY_FUNCTION__);\
            log->log((type), __VA_ARGS__);\
        }

#define INFO(...) {\
            log->set_debug(nullptr, 0, nullptr);\
            log->log(llama::Logger::Info, __VA_ARGS__);\
        }

#define WARN(...) {\
            LOG(llama::Logger::Warning, __VA_ARGS__);\
        }

#define SYNTAXWARN(...) {\
            LOG(llama::Logger::Warning, __VA_ARGS__);\
        }

#define RUNTIMEERROR(...) {\
            LOG(llama::Logger::RuntimeError, __VA_ARGS__);\
        }

#define SYNTAXERROR(...) {\
            LOG(llama::Logger::SyntaxError, __VA_ARGS__);\
        }

#define TYPEERROR(...) {\
            LOG(llama::Logger::TypeError, __VA_ARGS__);\
        }

#define PANIC(...) {\
            LOG(llama::Logger::Panic, __VA_ARGS__);\
        }

namespace llama {
    enum Status {
        Ok, 
        Failure, 
    };
    
    class LogSnippet {
    public:
        LogSnippet();
        LogSnippet(std::string str, size_t pos);
        ~LogSnippet();

        size_t start;
        size_t end;
        size_t line;
        size_t collumn;
    };

    class Logger {
    public:
        enum Type {
            Info, 
            Warning, 
            SyntaxError, 
            RuntimeError, 
            TypeError, 
            Panic, 
        };

        Logger();
        ~Logger();

        void reset();
        void set_source(const char * file);
        void set_snippet(LogSnippet m_snippet);
        void set_debug(const char * file, int line, const char * function);
        void set_recoverable();

        void log(Type type, const char * fmt, ...);

        bool has_error();

        std::string dump();
    private:
        const char * source;
        LogSnippet   snippet;
        std::string  err;

        bool recoverable;

        const char * __d_file;
        const char * __d_func;
        int          __d_line;
    };
}

#endif