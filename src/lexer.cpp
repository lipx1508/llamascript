/* -=============
     Includes
   =============- */

#include <lexer.h>
#include <error.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cctype>
#include <clocale>
#include <string>
#include <vector>
#include <stack>
#include <map>

/* -===============
     Internals
   ================- */

namespace llama {
    static std::map<std::string, Token::Type> keywords = {
        { "not",      Token::Type::Not      }, 
        { "or",       Token::Type::Or       }, 
        { "and",      Token::Type::And      }, 
        { "var",      Token::Type::Var      }, 
        { "let",      Token::Type::Let      }, 
        { "const",    Token::Type::Const    }, 
        { "extends",  Token::Type::Extends  }, 
        { "if",       Token::Type::If       }, 
        { "else",     Token::Type::Else     }, 
        { "for",      Token::Type::For      }, 
        { "while",    Token::Type::While    }, 
        { "loop",     Token::Type::Loop     }, 
        { "do",       Token::Type::Do       }, 
        { "ref",      Token::Type::Ref      }, 
        { "as",       Token::Type::As       }, 
        { "repeat",   Token::Type::Repeat   }, 
        { "break",    Token::Type::Break    }, 
        { "return",   Token::Type::Return   }, 
        { "null",     Token::Type::Null     }, 
        { "true",     Token::Type::True     }, 
        { "false",    Token::Type::False    }, 
        { "fn",       Token::Type::Fn       }, 
        { "int",      Token::Type::Int      }, 
        { "float",    Token::Type::Float    }, 
        { "bool",     Token::Type::Bool     }, 
        { "class",    Token::Type::Class    }, 
        { "import",   Token::Type::Import   }, 
        { "export",   Token::Type::Export   }, 
    };

    static std::map<Token::Type, std::string> ops = {
        { Token::Type::Plus,     "+" }, 
        { Token::Type::Minus,    "-" }, 
        { Token::Type::Multiply, "*" }, 
        { Token::Type::Divide,   "/" }, 
        { Token::Type::Modulo,   "%" }, 
        { Token::Type::Comma,    "," }, 
        { Token::Type::Colon,    ":" }, 
        { Token::Type::Dot,      "." }, 
        { Token::Type::LParen,   "(" }, 
        { Token::Type::RParen,   ")" }, 
        { Token::Type::LBrace,   "{" }, 
        { Token::Type::RBrace,   "}" }, 
        { Token::Type::LBracket, "[" }, 
        { Token::Type::RBracket, "]" }, 
        { Token::Type::End,      ";" }, 
        { Token::Type::Not,      "!" }, 
        { Token::Type::Greater,  ">" }, 
        { Token::Type::Lesser,   "<" }, 
        { Token::Type::Equal,    "=" }, 
    };

    static inline char to_upper(char c) {
        if (c >= 'a' && c <= 'z') c += 32;
        return c;
    }

    static inline char to_lower(char c) {
        if (c >= 'A' && c <= 'Z') c -= 32;
        return c;
    }

    static inline bool is_space(char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    static inline bool is_digit(char c) {
        return c >= '0' && c <= '9';
    }

    static inline bool is_hex(char c) {
        c = to_lower(c);
        return c >= 'a' && c <= 'f';
    }

    static inline bool is_hexdigit(char c) {
        return is_digit(c) || is_hex(c);
    }

    static inline bool is_octal(char c) {
        return c >= '0' && c <= '7';
    }

    static inline bool is_binary(char c) {
        // I'm writing this for portability but it's batshit insane lmao
        return c == '0' || c == '1';
    }

    static inline bool is_op(char c) {
        return c == '+' || c == '-' || c == '/'  || c == '%' || c == ',' || c == ':' || c == '.' || c == '(' || 
               c == ')' || c == '{' || c == '}' || c == '['  || c == ']' || c == ';' || c == '*' || c == '!' || 
               c == '>' || c == '<' || c == '=';
    }

    static inline bool is_str(char c) {
        return c == '\'' || c == '\"' || c == '`';
    }

    static inline bool is_literal(char c) {
        c = to_lower(c);
        return c == 'x' || c == 'o' || c == 'b';
    }

    static inline bool is_label(char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == '$';
    }

    static inline bool is_ascii(char c) {
        return (unsigned char)(c) < 0x7f;
    }

    static inline bool is_unary(char c) {
        return c == '+' || c == '-';
    }

    static inline bool is_comment(char c) {
        // I can't believe I have to really make this a function <|:(
        return c == '/';
    }

    static inline int hex2int(char c) {
        if (is_digit(c)) return c - '0';
        if (is_hex(c))   return c - 'a' + 10;
        return -1;
    }
}

/* -=================
     Token class
   =================- */

/* -=- (Con/des)tructors -=- */
llama::Token::Token(Type m_type, LogSnippet m_snippet) {
    lexeme.clear();
    type    = m_type;
    snippet = m_snippet;
}

/* -=- Abstractions -=- */
bool llama::Token::is_op() {
    return type >= Type::Plus && type <= Type::RBrace;
}

bool llama::Token::is_arithmetic() {
    return type >= Type::Plus && type <= Type::Power;
}

bool llama::Token::is_logical() {
    return (type >= Type::Equals && type <= Type::LeEquals) || type == Type::And || type == Type::Or;
}

bool llama::Token::is_special() {
    return type == Type::As || type == Type::Ref || (type >= Type::Comma && type <= Token::Type::Dot);
}

bool llama::Token::is_unary() {
    return type == Type::UnaryPlus || type == Type::UnaryMinus || type == Type::Not;
}

bool llama::Token::is_access() {
    return type == Token::Type::Dot;
}

bool llama::Token::is_expr() {
    return is_arithmetic() || is_logical() || is_special() || is_unary();
}

bool llama::Token::is_internal() {
    return type == Type::CallEnd || type == Type::CallStart;
}

bool llama::Token::is_lscope() {
    return type == Type::LParen || type == Type::LBrace || type == Type::LBracket;
}

bool llama::Token::is_rscope() {
    return type == Type::RParen || type == Type::RBrace || type == Type::RBracket;
}

bool llama::Token::is_operand() {
    return type == Type::Label  || type == Type::Integer   || type == Type::Decimal   || 
           type == Type::String || type == Type::RawString || type == Type::Character || 
           type == Type::Null   || type == Type::True      || type == Type::False;
}

bool llama::Token::is_callable() {
    return type == Token::Type::Label  || type == Token::Type::String || type == Token::Type::RParen || 
           type == Token::Type::RBracket;
}

bool llama::Token::is_single() {
    return type == Type::Repeat || type == Type::Break;
}

bool llama::Token::is_double() {
    return type == Type::Return || type == Type::Import || type == Type::Export;
}

bool llama::Token::is_keyword() {
    return type == Type::And || type >= Type::Or || type >= Type::Not || 
           (type >= Type::Var && type <= Type::Export);
}

bool llama::Token::is_primitive() {
    return type >= Type::Fn && type <= Type::Class;
}

bool llama::Token::is_decl() {
    return type >= Type::Var && type <= Type::Const;
}

bool llama::Token::is_empty() {
    return lexeme.empty() && type == Type::Unknown;
}

/* -=- Utils -=- */
void llama::Token::clear() {
    lexeme.clear();
    type = Type::Unknown;
}

int llama::Token::precedence() {
    switch (type) {
        case Type::Colon:     return 1;
        case Type::Comma:     return 2;
        case Type::Or:        return 3;
        case Type::And:       return 4;
        case Type::Greater:   return 5;
        case Type::Lesser:    return 5;
        case Type::GrEquals:  return 5;
        case Type::LeEquals:  return 5;
        case Type::Equals:    return 6;
        case Type::NotEquals: return 6;
        case Type::Plus:      return 8;
        case Type::Minus:     return 8;
        case Type::Multiply:  return 9;
        case Type::Divide:    return 9;
        case Type::Modulo:    return 9;
        case Type::Power:     return 10;
        case Type::Not:       return 11;
        case Type::As:        return 12;
        //case Type::CallStart: return 13;
        //case Type::CallEnd:   return 13;
        case Type::Dot:       return 14;
        case Type::Equal:     return 15;
        default:              return -1;
    }
}

bool llama::Token::associativity() {
    // If true it means right associativity, else left associativity
    return type == Type::Power;
}

llama::Token llama::Token::reverse() {
    // Inverts scoped token types
    Token token = * this;

    switch (type) {
        case Type::LParen:    { token.type = Type::RParen;    break; }
        case Type::LBrace:    { token.type = Type::RBrace;    break; }
        case Type::LBracket:  { token.type = Type::RBracket;  break; }
        case Type::RParen:    { token.type = Type::LParen;    break; }
        case Type::RBrace:    { token.type = Type::LBrace;    break; }
        case Type::RBracket:  { token.type = Type::LBracket;  break; }
        case Type::True:      { token.type = Type::False;     break; }
        case Type::False:     { token.type = Type::True;      break; }
        case Type::CallEnd:   { token.type = Type::CallStart; break; }
        case Type::CallStart: { token.type = Type::CallEnd;   break; }
        default: break;
    }

    token.lexeme = ops[token.type];

    return token;
}

const char * llama::Token::type_str() {
    // Returns the raw name of the type since C++ doesn't provide it natively smh
    switch (type) {
        case Type::Label:      return "Type::Label";
        case Type::Integer:    return "Type::Integer";
        case Type::Decimal:    return "Type::Decimal";
        case Type::String:     return "Type::String";
        case Type::RawString:  return "Type::RawString";
        case Type::Character:  return "Type::Character";
        case Type::Plus:       return "Type::Plus";
        case Type::Minus:      return "Type::Minus";
        case Type::Multiply:   return "Type::Multiply";
        case Type::Divide:     return "Type::Divide";
        case Type::Modulo:     return "Type::Modulo";
        case Type::Power:      return "Type::Power";
        case Type::Equal:      return "Type::Equal";
        case Type::Comma:      return "Type::Comma";
        case Type::Colon:      return "Type::Colon";
        case Type::Dot:        return "Type::Dot";
        case Type::Equals:     return "Type::Equals";
        case Type::Greater:    return "Type::Greater";
        case Type::Lesser:     return "Type::Lesser";
        case Type::NotEquals:  return "Type::NotEquals";
        case Type::GrEquals:   return "Type::GrEquals";
        case Type::LeEquals:   return "Type::LeEquals";
        case Type::Not:        return "Type::Not";
        case Type::Or:         return "Type::Or";
        case Type::And:        return "Type::And";
        case Type::LParen:     return "Type::LParen";
        case Type::RParen:     return "Type::RParen";
        case Type::LBrace:     return "Type::LBrace";
        case Type::RBrace:     return "Type::RBrace";
        case Type::LBracket:   return "Type::LBracket";
        case Type::RBracket:   return "Type::RBracket";
        case Type::End:        return "Type::End";
        case Type::Var:        return "Type::Var";
        case Type::Let:        return "Type::Let";
        case Type::Const:      return "Type::Const";
        case Type::Extends:    return "Type::Extends";
        case Type::If:         return "Type::If";
        case Type::Else:       return "Type::Else";
        case Type::For:        return "Type::For";
        case Type::While:      return "Type::While";
        case Type::Loop:       return "Type::Loop";
        case Type::Do:         return "Type::Do";
        case Type::Ref:        return "Type::Ref";
        case Type::As:         return "Type::As";
        case Type::Repeat:     return "Type::Repeat";
        case Type::Break:      return "Type::Break";
        case Type::Return:     return "Type::Return";
        case Type::Null:       return "Type::Null";
        case Type::True:       return "Type::True";
        case Type::False:      return "Type::False";
        case Type::Fn:         return "Type::Fn";
        case Type::Int:        return "Type::Int";
        case Type::Float:      return "Type::Float";
        case Type::Bool:       return "Type::Bool";
        case Type::Class:      return "Type::Class";
        case Type::Import:     return "Type::Import";
        case Type::Export:     return "Type::Export";
        case Type::UnaryPlus:  return "Type::UnaryPlus";
        case Type::UnaryMinus: return "Type::UnaryMinus";
        case Type::CallEnd:  return "Type::CallEnd";
        case Type::CallStart:    return "Type::CallStart";
        default:               return "Type::Unknown";
    }
}

/* -=================
     Lexer class
   =================- */

/* -=- (Con/des)tructors -=- */
llama::Lexer::Lexer() {
    setlocale(LC_ALL, "en_US.UTF-8");
}

llama::Lexer::~Lexer() {
    tokens.clear();
}

/* -=- Base functions -=- */
void llama::Lexer::parse(Logger * m_log, std::string m_str) {
    log = m_log;
    str = m_str;

    read_str();
    refactor();
}

void llama::Lexer::dump() {
    for (size_t i = 0; i < tokens.size(); ++i) {
        printf("Token %zu: '%s' (type = %s)\n", i, tokens[i].lexeme.c_str(), tokens[i].type_str());
        if (tokens[i].type == Token::Type::LBrace) {
            printf("- Scope start -\n");
        } else if (tokens[i].type == Token::Type::RBrace || tokens[i].type == Token::Type::End) {
            printf("- Scope end -\n");
        }
    }
    printf("-- Dump finished --\n");
}

/* -=- Primitive search -=- */
void llama::Lexer::read_str() {
    // Reads the input appropriately
    size_t i = 0;
    while (i < str.length()) {
        // Avoids infinite loops
        size_t old_i = i;

        char c = str[i];
        if (is_space(c)) {
            // Checkes for whitespaces
            ++i;
            continue;
        } else if (is_comment(c) && (seek(i + 1) == '*' || seek(i + 1) == '/')) {
            // Checkes for comments
            i = read_comment(i);
        } else if (is_digit(c) || (c == '.' && is_digit(seek(i + 1)))) {
            // Checkes for numbers
            i = read_number(i);
        } else if (is_str(c)) {
            // Checkes for strings
            i = read_string(i);
        } else if (is_op(c)) {
            // Checkes for operators
            i = read_op(i);
        } else if (is_label(c) || !is_ascii(c)) {
            // Checkes for labels
            i = read_label(i);
        } else {
            // If any of the options didn't match
            log->set_snippet(LogSnippet(str, i));
            SYNTAXERROR("unexpected character %c", c);
            i = ERROR_IDX;
        }

        if (i == old_i) {
            log->set_snippet(LogSnippet(str, i));
            SYNTAXERROR("infinite loop, aborting");
            break;
        } else if (i == ERROR_IDX) {
            break;
        }
    }
}

size_t llama::Lexer::read_comment(size_t start) {
    size_t i = start + 2;

    // Comment flags
    bool lined   = (str[start] == '/' && seek(start + 1) == '/');
    bool mlined  = (str[start] == '/' && seek(start + 1) == '*');
    bool has_end = false;

    // Seeks for a comment (pretty much the same logic as the read_string() function)
    while (i < str.length()) {
        bool single_end = (lined  && str[i] == '\n');
        bool multi_end  = (mlined && str[i] == '*' && seek(i + 1) == '/');

        if (single_end || multi_end) {
            has_end = true;
            break;
        }

        ++i;
    }

    if (lined) has_end = true;

    if (!has_end) {
        log->set_snippet(LogSnippet(str, start));
        SYNTAXERROR("unterminated comment");
        return ERROR_IDX;
    }

    return i + 1 + mlined;
}

size_t llama::Lexer::read_string(size_t start) {
    Token token = Token(Token::Type::String);

    size_t i = start + 1;

    char first = str[start];

    // String flags
    bool has_end = false;

    // Seeks for a string until it ends
    while (i < str.length()) {
        if (str[i] == first) {
            // If the string has ended
            has_end = true;
            break;
        }

        token.lexeme.push_back(str[i]);

        ++i;
    }

    if (!has_end) {
        log->set_snippet(LogSnippet(str, start));
        SYNTAXERROR("unterminated string");
        return ERROR_IDX;
    }

    push(start, token);

    return i + 1;
}

size_t llama::Lexer::read_number(size_t start) {
    Token token = Token(Token::Type::Integer);

    size_t end = start;

    // Checkes if the number is a literal or a decimal number
    char lit = seek(start + 1);
    if (str[start] == '0' && is_literal(lit)) {
        switch (lit) {
            case 'x': {
                end = search_hexadecimal(start + 2);
                break;
            }
            case 'o': {
                end = search_octal(start + 2);
                break;
            }
            case 'b': {
                end = search_binary(start + 2);
                break;
            }
            default: {
                log->set_snippet(LogSnippet(str, start + 1));
                SYNTAXERROR("invalid literal %c", lit);
                return ERROR_IDX;
            }
        }
    } else {
        bool has_dot = false;
        end = search_decimal(start, has_dot);
        if (has_dot) token.type = Token::Type::Decimal;
    }

    // Check for errors
    if (end != ERROR_IDX && end > start) {
        token.lexeme = str.substr(start, end - start);
        push(start, token);
    }

    return end;
}

size_t llama::Lexer::read_op(size_t start) {
    Token token = Token(Token::Type::Unknown);

    size_t i = start;
    
    // Checkes for operators
    char c = str[i];
    switch (c) {
        case '+': { token.type = Token::Type::Plus;     break; }
        case '-': { token.type = Token::Type::Minus;    break; }
        case '*': { token.type = Token::Type::Multiply; break; }
        case '/': { token.type = Token::Type::Divide;   break; }
        case '%': { token.type = Token::Type::Modulo;   break; }
        case ',': { token.type = Token::Type::Comma;    break; }
        case ':': { token.type = Token::Type::Colon;    break; }
        case '.': { token.type = Token::Type::Dot;      break; }
        case '(': { token.type = Token::Type::LParen;   break; }
        case ')': { token.type = Token::Type::RParen;   break; }
        case '{': { token.type = Token::Type::LBrace;   break; }
        case '}': { token.type = Token::Type::RBrace;   break; }
        case '[': { token.type = Token::Type::LBracket; break; }
        case ']': { token.type = Token::Type::RBracket; break; }
        case ';': { token.type = Token::Type::End;      break; }
        case '!': { token.type = Token::Type::Not;      break; }
        case '>': { token.type = Token::Type::Greater;  break; }
        case '<': { token.type = Token::Type::Lesser;   break; }
        case '=': { token.type = Token::Type::Equal;    break; }
        default: break;
    }

    token.lexeme.push_back(c);

    char lc = seek(i + 1);
    if (is_op(lc)) {
        switch (lc) {
            case '*': {
                if (c == '*') token.type = Token::Type::Power;
                else break;
                
                token.lexeme.push_back(lc);
                ++i;
                break;
            }
            case '=': {
                if (c == '>') token.type = Token::Type::GrEquals;
                else if (c == '<') token.type = Token::Type::LeEquals;
                else if (c == '=') token.type = Token::Type::Equals;
                else if (c == '!') token.type = Token::Type::NotEquals;
                else break;
                
                token.lexeme.push_back(lc);
                ++i;
                break;
            }
            default: break;
        }
    }
    
    if (token.type == Token::Type::Unknown) {
        log->set_snippet(LogSnippet(str, i));
        SYNTAXERROR("unknown operator %c", c);
        return ERROR_IDX;
    }

    push(start, token);

    return i + 1;
}

size_t llama::Lexer::read_label(size_t start) {
    Token token = Token(Token::Type::Label);

    size_t i = start;
    
    // Checkes for labels
    while (i < str.length()) {
        if (is_label(str[i]) || is_digit(str[i])) {
            // Handles labels
            token.lexeme.push_back(str[i]);
            ++i;
            continue;
        } else if (is_space(str[i]) || is_op(str[i])) {
            // Stop reading in case another valid token type is found
            break;
        } else if (!is_ascii(str[i])) {
            // In case a unicode character is found
            log->set_snippet(LogSnippet(str, start));
            SYNTAXERROR("special UTF-8 characters are not allowed for labels");
            return ERROR_IDX;
        } else {
            // In case any of the conditions are not met properly
            log->set_snippet(LogSnippet(str, i));
            SYNTAXERROR("unexpected character '%c' in label", str[i]);
            return ERROR_IDX;
        }
    }

    push(start, token);

    return i;
}

/* -=- Deep index-oriented search -=- */
size_t llama::Lexer::search_decimal(size_t start, bool & has_dot) {
    size_t i = start;

    // Number flags
    bool has_e   = false;
    bool has_end = false;
    bool has_sig = false;

    // Searches for the last character of a decimal number
    while (i < str.length()) {
        if (is_unary(str[i]) && has_e) {
            // Handles unary numbers for the e-notation exponent
            if (has_sig) {
                log->set_snippet(LogSnippet(str, i));
                SYNTAXERROR("duplicated signal on e-notation exponent");
                return ERROR_IDX;
            } else {
                has_sig = true;
                ++i;
                continue;
            }
        }

        if (is_digit(str[i]) || str[i] == '_') {
            // Handles digits and the '_' number separator
            ++i;

            // Also ends e-notation if avaliable
            if (has_e) has_end = true;
            
            continue;
        } else if (to_lower(str[i]) == 'e') {
            // Handles the e-notation marker
            if (has_e) {
                log->set_snippet(LogSnippet(str, i));
                SYNTAXERROR("duplicated e-notation marker in number");
                return ERROR_IDX;
            } else {
                has_e = true;
                ++i;
                continue;
            }
        } else if (str[i] == '.') {
            // Handles decimal numbers
            if (has_e) {
                log->set_snippet(LogSnippet(str, i));
                SYNTAXERROR("decimal number on e-notation exponent");
                return ERROR_IDX;
            }

            if (has_dot) {
                log->set_snippet(LogSnippet(str, i));
                SYNTAXERROR("duplicated dot in number");
                return ERROR_IDX;
            } else {
                has_dot = true;
                ++i;
                continue;
            }
        } else if (is_space(str[i]) || is_op(str[i])) {
            // Checkes if the e-notation number ended properly if it exists
            if (has_e && !has_end) {
                log->set_snippet(LogSnippet(str, i));
                SYNTAXERROR("invalid e-notation exponent");
                return ERROR_IDX;
            }

            // Stop reading in case another valid token type is found
            break;
        } else {
            // In case any of the conditions are not met properly
            log->set_snippet(LogSnippet(str, i));
            SYNTAXERROR("malformed number");
            return ERROR_IDX;
        }
    }

    return i;
}

size_t llama::Lexer::search_hexadecimal(size_t start) {
    size_t i = start;

    // Searches for the last character of a hexadecimal number
    while (i < str.length()) {
        if (is_hexdigit(str[i]) || str[i] == '_') {
            // Handles digits and the '_' number separator
            ++i;
            continue;
        } else if (is_space(str[i]) || is_op(str[i])) {
            // Stop reading in case another valid token type is found
            break;
        } else {
            // In case any of the conditions are not met properly
            log->set_snippet(LogSnippet(str, i));
            SYNTAXERROR("hexadecimal number containing non-hexadecimal character %c", str[i]);
            return ERROR_IDX;
        }
    }

    return i;
}

size_t llama::Lexer::search_octal(size_t start) {
    size_t i = start;

    // Searches for the last character of a hexadecimal number
    while (i < str.length()) {
        if (is_octal(str[i]) || str[i] == '_') {
            // Handles digits and the '_' number separator
            ++i;
            continue;
        } else if (is_space(str[i]) || is_op(str[i])) {
            // Stop reading in case another valid token type is found
            break;
        } else {
            // In case any of the conditions are not met properly
            log->set_snippet(LogSnippet(str, i));
            SYNTAXERROR("octal number containing non-octal character %c", str[i]);
            return ERROR_IDX;
        }
    }

    return i;
}

size_t llama::Lexer::search_binary(size_t start) {
    size_t i = start;

    // Searches for the last character of a hexadecimal number
    while (i < str.length()) {
        if (is_binary(str[i]) || str[i] == '_') {
            // Handles digits and the '_' number separator
            ++i;
            continue;
        } else if (is_space(str[i]) || is_op(str[i])) {
            // Stop reading in case another valid token type is found
            break;
        } else {
            // In case any of the conditions are not met properly
            log->set_snippet(LogSnippet(str, i));
            SYNTAXERROR("binary number containing non-binary character %c", str[i]);
            return ERROR_IDX;
        }
    }

    return i;
}

/* -=- Refactoring -=- */
void llama::Lexer::refactor() {
    // Scans for HEAVILY specific errors so the analyser receives something that makes sense
    std::stack<Token> expects;

    Token last;

    bool is_fn = false;

    // First iteration
    size_t i = 0;
    while (i < tokens.size()) {
        Token token = tokens[i];

        if (token.is_internal()) {
            ++i;
            continue;
        }

        if (token.type == Token::Type::Label && keywords.find(token.lexeme) != keywords.end()) {
            token.type = keywords[token.lexeme];
            if (token.type == Token::Type::Fn) is_fn = true;
        }

        if (!is_fn && token.is_callable() && seek_token(i + 1).type == Token::Type::LParen) {
            tokens.insert(tokens.begin() + i + 1, Token(Token::Type::CallStart, token.snippet));
            expects.push(Token(Token::Type::CallEnd, token.snippet));
        }

        // printf("going... token '%s' found at %zu (last '%s', type = %s)\n", token.lexeme.c_str(), i, last.lexeme.c_str(), last.type_str());

        if (token.is_op() && seek_token(i + 1).is_operand() && !seek_token(i - 1).is_operand()) {
            if (token.type == Token::Type::Plus) {
                token.type = Token::Type::UnaryPlus;
            } else if (token.type == Token::Type::Minus) {
                token.type = Token::Type::UnaryMinus;
            }
        }

        if (token.is_expr() && (last.is_arithmetic() || last.is_special())) {
            log->set_snippet(token.snippet);
            SYNTAXERROR("unexpected operator '%s'", token.lexeme.c_str());
            return;
        } else if (token.is_lscope()) {
            expects.push(token.reverse());
            is_fn = false;
        } else if (token.is_rscope()) {
            if (expects.size() > 0) {
                if (expects.top().type == token.type) {
                    expects.pop();
                } else {
                    log->set_snippet(token.snippet);
                    SYNTAXERROR("unexpected token '%s', expected '%s'", token.lexeme.c_str(), ops[expects.top().type].c_str());
                    return;
                }
            } else {
                log->set_snippet(token.snippet);
                SYNTAXERROR("unmatched token '%s'");
                return;
            }

            if (expects.size() > 0 && expects.top().type == Token::Type::CallEnd) {
                tokens.insert(tokens.begin() + i + 1, expects.top());
                expects.pop();
            }
        }

        if (!token.is_internal()) last = token;

        tokens[i] = token;

        ++i;
    }

    if (expects.size() > 0) {
        log->set_snippet(expects.top().snippet);
        SYNTAXERROR("unmatched token '%s'", expects.top().reverse().lexeme.c_str());
    }
}

/* -=- String and token management -=- */
char llama::Lexer::seek(size_t pos) {
    // Seeks for characters that can be also AHEAD of the size of the string
    // NOTE: doing this because it reduces code + makes more idiotic and focused on what really matters
    if (pos >= str.length()) return '\0';
    return str[pos];
}

llama::Token llama::Lexer::seek_token(size_t pos) {
    // Does the exact same as the seek() function but for tokens
    if (pos >= tokens.size()) return Token(Token::Type::Unknown);
    return tokens[pos];
}

void llama::Lexer::push(size_t pos, Token token) {
    if (!token.is_empty()) {
        token.snippet = LogSnippet(str, pos);
        tokens.push_back(token);
    }
}