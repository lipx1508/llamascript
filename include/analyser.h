#ifndef LLAMA_ANALYSER_H
#define LLAMA_ANALYSER_H

#include <lexer.h>
#include <ir.h>
#include <module.h>

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <stack>

namespace llama {
    class Analyser {
    public:
        Analyser();
        ~Analyser();

        void read(Module * m_mod, Logger * m_log, Lexer * m_lex);
        
        void dump();
    private:
        size_t parse_statement(size_t pos);
        size_t parse_scope(size_t pos, bool initialize = true);
        size_t parse_if(size_t pos);
        size_t parse_while(size_t pos);
        size_t parse_for(size_t pos);
        size_t parse_loop(size_t pos);
        size_t parse_fn(size_t pos, bool expr = false);
        // size_t parse_class(size_t pos);
        // size_t parse_class_field(size_t pos, ClassDB & c);
        size_t parse_single(size_t pos);
        size_t parse_double(size_t pos);
        size_t parse_declaration(size_t pos, bool can_assign = true, Token::Type end = Token::Type::End);
        size_t parse_expr(size_t pos, bool can_assign = true, bool is_decl = false, Token::Type end = Token::Type::End);
        
        Token seek_token(size_t pos);

        Module    * mod;
        Lexer     * lex;
        Logger    * log;
        IRBuilder * ir;
    };
}

#endif