#ifndef LLAMA_LEXER_H
#define LLAMA_LEXER_H

#include <error.h>

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace llama {
    class Token {
    public:
        enum class Type {
            Unknown, 

            Label, 
            Integer, 
            Decimal, 
            String, 
            RawString, 
            Character, 

            Plus, 
            Minus, 
            Multiply, 
            Divide, 
            Modulo, 
            Power, 
            Equal, 
            
            Comma, 
            Colon, 
            Dot, 

            Equals, 
            Greater, 
            Lesser, 
            NotEquals, 
            GrEquals, 
            LeEquals, 
            Not, 
            Or, 
            And, 

            LParen, 
            RParen, 
            LBrace, 
            RBrace, 
            LBracket, 
            RBracket, 

            End, 

            Var, 
            Let, 
            Const, 
            Extends, 

            If, 
            Else, 
            For, 
            While, 
            Loop, 
            Do, 

            Ref, 
            As, 

            Repeat, 
            Break, 
            Return, 

            Null, 
            True, 
            False, 
            Fn, 
            Int, 
            Float, 
            Bool, 
            Class, 

            Import, 
            Export, 

            // Specific for the analyser
            UnaryPlus, 
            UnaryMinus, 
            CallStart, 
            CallEnd, 
        };

        Token(Type m_type = Type::Unknown, LogSnippet m_snippet = LogSnippet());

        Type        type;
        std::string lexeme;
        LogSnippet  snippet;

        bool is_op();
        bool is_arithmetic();
        bool is_logical();
        bool is_special();
        bool is_unary();
        bool is_access();
        bool is_expr();
        bool is_internal();
        bool is_lscope();
        bool is_rscope();
        bool is_operand();
        bool is_callable();
        bool is_single();
        bool is_double();
        bool is_keyword();
        bool is_primitive();
        bool is_decl();
        bool is_empty();
        void clear();

        int   precedence();
        bool  associativity();
        Token reverse();

        const char * type_str();
    };

    class Analyser;

    class Lexer {
    public:
        Lexer();
        ~Lexer();

        void parse(Logger * m_log, std::string m_str);

        void dump();
    private:
        void   read_str();
        size_t read_comment(size_t start);
        size_t read_number(size_t start);
        size_t read_string(size_t start);
        size_t read_op(size_t start);
        size_t read_label(size_t start);

        size_t search_decimal(size_t start, bool & has_dot);
        size_t search_hexadecimal(size_t start);
        size_t search_octal(size_t start);
        size_t search_binary(size_t start);

        void  refactor();
        Token refactor_keyword(Token token, size_t pos);

        char  seek(size_t pos);
        Token seek_token(size_t pos);
        void  push(size_t pos, Token token);

        std::string        str;
        std::vector<Token> tokens;

        Logger * log;

        friend Analyser;
    };
}

#endif