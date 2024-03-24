/* -=============
     Includes
   =============- */

#include <analyser.h>
#include <lexer.h>
#include <ir.h>
#include <module.h>
#include <error.h>
#include <util.h>

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <string>
#include <vector>
#include <list>
#include <map>

/* -===================
     Analyser class
   ===================- */

/* -=- (Con/des)tructors -=- */
llama::Analyser::Analyser() {}
llama::Analyser::~Analyser() {}

/* -=- Base functions -=- */
void llama::Analyser::read(Module * m_mod, Logger * m_log, Lexer * m_lex) {
    mod = m_mod;
    log = m_log;
    lex = m_lex;

    ir = new IRBuilder();
    ir->set_module(m_mod);

    FunctionEntry func;
    if (parse_scope(0, false) != ERROR_IDX) {
        size_t func_idx = mod->get_functions()->add(func);
        ir->build(mod->get_functions()->at(func_idx)->get_data());
    }

    delete ir;
}

/* -=- Statement cases -=- */
size_t llama::Analyser::parse_statement(size_t pos) {
    INFO("analysing a statement at %zu", pos);

    size_t i = pos;

    Token token = seek_token(i);

    if (token.is_expr() || token.is_operand()) {
        i = parse_expr(i);
    } else if (token.is_decl()) {
        i = parse_declaration(i);
    } else if (token.is_single()) {
        i = parse_single(i);
    } else if (token.is_double()) {
        i = parse_double(i);
    } else {
        switch (token.type) {
            case Token::Type::LBrace: {
                i = parse_scope(i + 1);
                break;
            }
            case Token::Type::If: {
                i = parse_if(i + 1);
                break;
            }
            case Token::Type::While: {
                i = parse_while(i + 1);
                break;
            }
            case Token::Type::Loop: {
                i = parse_loop(i + 1);
                break;
            }
            case Token::Type::Class: {
                // i = parse_class(i + 1);
                break;
            }
            case Token::Type::Fn: {
                i = parse_fn(i + 1);
                break;
            }
            case Token::Type::End: {
                ++i;
                break;
            }
            case Token::Type::Else: {
                log->set_snippet(token.snippet);
                SYNTAXERROR("else statement without a matching if statement");
                return ERROR_IDX;
            }
            default: {
                log->set_snippet(token.snippet);
                SYNTAXERROR("unknown token type %s at %zu", token.type_str(), i);
                return ERROR_IDX;
            }
        }
    }

    return i;
}

size_t llama::Analyser::parse_scope(size_t pos, bool initialize) {
    INFO("analysing a scope block at %zu (initialize=%s)", pos, BOOLALPHA(initialize));

    if (initialize) ir->push_block();
    
    size_t i = pos;
    while (i < lex->tokens.size()) {
        if (lex->tokens[i].type == Token::Type::RBrace) break;

        i = parse_statement(i);
        if (i == ERROR_IDX) return ERROR_IDX;
    }

    if (initialize) ir->end_block();

    return i + 1;
}

size_t llama::Analyser::parse_if(size_t pos) {
    INFO("analysing an if statement at %zu", pos);

    size_t i = parse_expr(pos, false, false, Token::Type::LBrace);
    if (i == ERROR_IDX) return ERROR_IDX;

    ir->push_if();
        i = parse_scope(i, false);
        if (i == ERROR_IDX) return ERROR_IDX;

        if (seek_token(i).type == Token::Type::Else) {
            ir->push_else();

            i = parse_scope(i + (seek_token(i + 1).type == Token::Type::LBrace ? 2 : 1), false);
            if (i == ERROR_IDX) return ERROR_IDX;
        }
    ir->end_block();
    
    return i;
}

size_t llama::Analyser::parse_while(size_t pos) {
    INFO("analysing a while statement at %zu", pos);
    ir->push_loop();
        size_t i = parse_expr(pos, false, false, Token::Type::LBrace);
        if (i == ERROR_IDX) return ERROR_IDX;

        ir->push_if();
            i = parse_scope(i, false);
            if (i == ERROR_IDX) return ERROR_IDX;

            ir->_repeat();
        ir->end_block();

        ir->_break();
    ir->end_block();
    
    return i;
}

size_t llama::Analyser::parse_loop(size_t pos) {
    INFO("analysing a loop at %zu", pos);
    ir->push_loop();
        size_t i = parse_scope(pos + 1, false);
        if (i == ERROR_IDX) return ERROR_IDX;
    ir->end_block();
    
    return i;
}

size_t llama::Analyser::parse_fn(size_t pos, bool expr) {
    INFO("analysing a function at %zu (expr=%s)", pos, BOOLALPHA(expr));

    FunctionEntry func;

    size_t i = pos;

    Token token = seek_token(i);
    if (token.type == Token::Type::Label) {
        if (mod->get_functions()->has(token.lexeme)) {
            log->set_snippet(token.snippet);
            SYNTAXERROR("the function %s already exists", token.lexeme.c_str());
            return ERROR_IDX;
        }

        func.set_name(token.lexeme);
        token = seek_token(++i);
    }

    if (token.type == Token::Type::LParen) {
        while (i < lex->tokens.size() && token.type != Token::Type::RParen) {
            token = seek_token(i);
            ++i;
        }
        ++i;
    } else {
        log->set_snippet(token.snippet);
        SYNTAXERROR("unexpected token '%s', expected '('", token.lexeme.c_str());
        return ERROR_IDX;
    }

    token = seek_token(i);

    if (token.type != Token::Type::LBrace) {
        IRBuilder * prev_ir = ir;
        IRBuilder   fn_ir   = IRBuilder();
        
        fn_ir.set_module(prev_ir->get_module());

        ir = &fn_ir;
        i  = parse_scope(i + 1, true);
        if (i != ERROR_IDX) {
            ir->build(func.get_data());
        }

        ir = prev_ir;
    } else {
        log->set_snippet(token.snippet);
        SYNTAXERROR("unexpected token '%s', expected block", token.lexeme.c_str());
        return ERROR_IDX;
    }
    
    func.set_line(seek_token(pos).snippet.line);

    size_t func_idx = mod->get_functions()->add(func);
    if (expr) return func_idx;

    if (!func.get_name().empty()) {
        ir->_newglobal(func.get_name());
        ir->_refglobal(func.get_name());
        ir->_pushfunc(func_idx);
        ir->_refset(-2);
        ir->_pop();
    }
    
    return i;
}

size_t llama::Analyser::parse_single(size_t pos) {
    INFO("analysing a single at %zu", pos);

    Token token = seek_token(pos);

    switch (token.type) {
        case Token::Type::Repeat: { ir->_repeat(); break; }
        case Token::Type::Break:  { ir->_break();  break; }
        default:                  return ERROR_IDX;
    }
    
    return pos + 2;
}

size_t llama::Analyser::parse_double(size_t pos) {
    INFO("analysing a double at %zu", pos);

    Token token = seek_token(pos);

    size_t i = pos;

    switch (token.type) {
        /*case Token::Type::Import: {
            token = seek_token(i + 1);
            if (token.type != Token::Type::Label) {
                log->set_snippet(token.snippet);
                SYNTAXERROR("import name should be a label");
                return ERROR_IDX;
            }
            if (ir->add_import(token.lexeme)) {
                log->set_snippet(token.snippet);
                SYNTAXWARN("'%s' was already imported", token.lexeme.c_str());
            }
            i += 2;
            break;
        }
        case Token::Type::Export: {
            token = seek_token(i + 1);
            if (token.type != Token::Type::Label) {
                log->set_snippet(token.snippet);
                SYNTAXERROR("export name should be a label");
                return ERROR_IDX;
            }
            if (ir->add_export(token.lexeme)) {
                log->set_snippet(token.snippet);
                SYNTAXWARN("'%s' was already exported", token.lexeme.c_str());
            }
            i += 2;
            break;
        }*/
        case Token::Type::Return: {
            token = seek_token(++i);
            if (token.is_operand()) {
                i = parse_expr(i, false);
                if (i == ERROR_IDX) return ERROR_IDX;
                ir->_return();
            } else if (token.type == Token::Type::End) {
                ++i;
                ir->_returnv();
            } else {
                log->set_snippet(token.snippet);
                SYNTAXERROR("return statement missing expression or ';'");
                return ERROR_IDX;
            }
            ++i;
            break;
        }
        default: return ERROR_IDX;
    }
    
    return i + 1;
}

size_t llama::Analyser::parse_declaration(size_t pos, bool can_assign, Token::Type end) {
    INFO("analysing a declaration at %zu (can_assign=%s, end=Token::%s)", pos, BOOLALPHA(can_assign), Token(end).type_str());

    size_t i = pos;

    Token token = seek_token(pos);

    std::string ident;

    switch (token.type) {
        case Token::Type::Const: // TODO: are constant variables REALLY worthy implementing?
        case Token::Type::Let: {
            if (seek_token(i + 1).type != Token::Type::Label) {
                log->set_snippet(token.snippet);
                SYNTAXERROR("missing identifier in declaration");
            }

            token = seek_token(++i);
            ir->_newlocal(token.lexeme);
            ident = token.lexeme;
            break;
        }
        case Token::Type::Var: {
            if (seek_token(i + 1).type != Token::Type::Label) {
                log->set_snippet(token.snippet);
                SYNTAXERROR("missing identifier in declaration");
            }

            token = seek_token(++i);

            ir->_newglobal(token.lexeme);
            ident = token.lexeme;
            break;
        }
        default: return ERROR_IDX;
    }

    i = parse_expr(i, true, true);

    return i;
}

size_t llama::Analyser::parse_expr(size_t pos, bool can_assign, bool is_decl, Token::Type end) {
    INFO("analysing an expression at %zu (can_assign=%s, is_decl=%s, end=Token::%s)", pos, BOOLALPHA(can_assign), BOOLALPHA(is_decl), Token(end).type_str());

    std::vector<Token> out, ops;

    bool has_end    = false;
    bool has_equal  = false;
    bool has_type   = false;
    bool has_arith  = false;
    bool has_access = false;
    
    size_t expr_start = pos;

    // Current token
    Token token;

    // For handling different types of tokens
    auto do_operand = [&](size_t i) {
        out.push_back(token);
        if (ops.size() > 0 && ops.back().is_unary()) {
            out.push_back(ops.back());
            ops.pop_back();
        }

        return i;
    };

    auto do_operator = [&](size_t i) {
        if (token.type == Token::Type::Dot && !has_equal) {
            if (is_decl) {
                log->set_snippet(token.snippet);
                SYNTAXERROR("cannot declare a property");
                return ERROR_IDX;
            }
            has_access = true;
        }

        // Pop all the remaining operators onto the output stack
        while (!ops.empty() && ops.back().type != Token::Type::LParen) {
            auto & op = ops.back();
            if ((op.precedence() == token.precedence() && token.associativity()) || 
                token.is_internal() || op.precedence() > token.precedence()) {
                out.push_back(ops.back());
                ops.pop_back();
            } else break;
        }

        if (token.is_internal()) out.push_back(token);
        else                     ops.push_back(token);

        if (token.is_arithmetic() || token.is_logical()) has_arith = true;

        return i;
    };

    auto do_rparen = [&](size_t i) {
        // Handles everything inside the parenthesis
        while (!ops.empty() && ops.back().type != Token::Type::LParen) {
            out.push_back(ops.back());
            ops.pop_back();

            if (ops.empty()) {
                // TODO: not handling this since it was checked during the refactoring (?)
                log->set_snippet(token.snippet);
                SYNTAXERROR("unmatched parenthesis");
                return ERROR_IDX;
            }
        }

        // Pops the remainder onto the output stack
        if (!ops.empty() && ops.back().type == Token::Type::LParen) {
            ops.pop_back();
        }

        return i;
    };

    auto do_scope = [&](size_t i) {
        // Pops all the remaining operators onto the output stack
        while (!ops.empty() && ops.back().type != Token::Type::LParen) {
            out.push_back(ops.back());
            ops.pop_back();
        }

        if (!ops.empty() && ops.back().type == Token::Type::LParen) {
            log->set_snippet(token.snippet);
            SYNTAXERROR("unmatched parenthesis");
            return ERROR_IDX;
        }

        return i;
    };

    auto specific_check = [&](size_t i, Token token) {
        switch (token.type) {
            case Token::Type::Colon: {
                if (!is_decl) {
                    log->set_snippet(token.snippet);
                    SYNTAXERROR("cannot specify a type here");
                    return ERROR_IDX;
                } else if (has_equal) {
                    log->set_snippet(token.snippet);
                    SYNTAXERROR("cannot specify a type after assignment");
                    return ERROR_IDX;
                } else if (has_type) {
                    log->set_snippet(token.snippet);
                    SYNTAXERROR("unexpected operator '%s', expected assignment or end", token.lexeme.c_str());
                    return ERROR_IDX;
                }

                while (i < lex->tokens.size()) {
                    Token::Type type = seek_token(i).type;
                    if (type == Token::Type::End || type == Token::Type::Equal) break;
                    ++i;
                }
                has_type = true;

                break;
            }
            case Token::Type::Comma: {
                out.push_back(token);
                i = do_scope(i);
                break;
            }
            case Token::Type::LParen: {
                ops.push_back(token);
                break;
            }
            case Token::Type::RParen: {
                i = do_rparen(i);
                break;
            }
            case Token::Type::Equal: {
                if (!can_assign) {
                    log->set_snippet(token.snippet);
                    SYNTAXERROR("assignment is forbidden at this scope");
                    return ERROR_IDX;
                }

                expr_start = i;

                has_equal = true;

                i = do_scope(i);
                out.push_back(token);
                break;
            }
            default: {
                log->set_snippet(token.snippet);
                SYNTAXERROR("unexpected token '%s' in expression", token.lexeme.c_str());
                return ERROR_IDX;
            }
        }

        return i;
    };

    // Iterates over all tokens
    size_t i = pos;
    while (i < lex->tokens.size() && !has_end) {
        token = lex->tokens[i];

        if (token.is_operand()) {
            i = do_operand(i);
        } else if (token.is_expr() || token.is_internal()) {
            i = do_operator(i);
        } else if (token.type == end) {
            if (expr_start > pos && expr_start + 1 == i) {
                log->set_snippet(token.snippet);
                SYNTAXERROR("expected expression");
                return ERROR_IDX;
            }

            has_end = true;
            break;
        } else {
            i = specific_check(i, token);
        }

        if (i == ERROR_IDX) return ERROR_IDX;

        ++i;
    }

    // Checkes for a missing ';'
    if (!has_end) {
        token = seek_token(pos);
        log->set_snippet(token.snippet);
        SYNTAXERROR("missing ';' operator");
        return ERROR_IDX;
    }

    // Checkes for a unmatched parenthesis
    while (!ops.empty()) {
        if (ops.back().type == Token::Type::LParen) {
            token = ops.back();
            log->set_snippet(token.snippet);
            SYNTAXERROR("unmatched parenthesis");
            return ERROR_IDX;
        }
        out.push_back(ops.back());
        ops.pop_back();
    }

    // Outputs the bytecode equivalent
    //printf("-- QUICK TOKEN DUMP --\n");
    //for (auto & t : out) printf("'%s': %s\n", t.lexeme.c_str(), t.type_str());

    Token last;
    
    bool is_assign = has_equal;
    bool is_global = true;
    bool is_call   = false;

    std::list<std::pair<size_t, std::string>> labels;

    std::stack<size_t> args_count;
    std::stack<bool>   no_comma;
    size_t             pop_count = 0;
    size_t             eq_count  = 0;

    // For transpiling tokens to bytecode
    auto transpile = [&](size_t i, Token & token) {
        if (token.is_expr() && !token.is_access()) {
            --pop_count;
            is_global = true;
        } else {
            ++pop_count;
        }

        switch (token.type) {
            case Token::Type::Null: {
                ir->_pushnull();
                if (!args_count.empty() && !no_comma.empty()) no_comma.top() = true;
                break;
            }
            case Token::Type::True: {
                ir->_pushtrue();
                if (!args_count.empty() && !no_comma.empty()) no_comma.top() = true;
                break;
            }
            case Token::Type::False: {
                ir->_pushfalse();
                if (!args_count.empty() && !no_comma.empty()) no_comma.top() = true;
                break;
            }
            case Token::Type::Integer: {
                ir->_pushint(std::stoi(token.lexeme.c_str()));
                if (!args_count.empty() && !no_comma.empty()) no_comma.top() = true;
                break;
            }
            case Token::Type::Decimal: {
                ir->_pushfloat(std::stod(token.lexeme.c_str()));
                if (!args_count.empty() && !no_comma.empty()) no_comma.top() = true;
                break;
            }
            case Token::Type::String: {
                ir->_pushstring(token.lexeme.c_str());
                if (!args_count.empty() && !no_comma.empty()) no_comma.top() = true;
                break;
            }
            case Token::Type::Label: {
                if (is_assign){
                    ir->_refglobal(token.lexeme);
                    --pop_count;
                } else {
                    ir->_getglobal(token.lexeme);
                }

                if (!is_global) labels.push_back(std::make_pair(ir->size() - 1, token.lexeme));
                is_global = false;

                if (!args_count.empty() && !no_comma.empty()) no_comma.top() = true;
                break;
            }
            case Token::Type::Equal: {
                labels.clear();
                eq_count  = pop_count;
                is_global = true;
                is_assign = false;
                break;
            }
            case Token::Type::Plus: {
                ir->_add();
                break;
            }
            case Token::Type::Minus: {
                ir->_sub();
                break;
            }
            case Token::Type::Multiply: {
                ir->_mul();
                break;
            }
            case Token::Type::Divide: {
                ir->_div();
                break;
            }
            case Token::Type::Modulo: {
                ir->_mod();
                break;
            }
            case Token::Type::Power: {
                ir->_pow();
                break;
            }
            case Token::Type::Dot: {
                if (labels.empty()) {
                    log->set_snippet(token.snippet);
                    SYNTAXERROR("unexpected token '.'");
                    return ERROR_IDX;
                }

                // I don't know what the fuck I've done here but it works so
                if (is_assign) ir->_refproperty(labels.front().second);
                else           ir->_getproperty(labels.front().second, -1);
                
                ir->set(ir->at(ir->size() - 1), labels.front().first);
                //INFO("instruction changed at %zu to %s", labels.back().first, ir->at(labels.front().first).dump().c_str());
                ir->pop();

                labels.pop_front();
                break;
            }
            case Token::Type::CallStart: {
                args_count.push(0);
                no_comma.push(false);
                is_call = true;
                break;
            }
            case Token::Type::CallEnd: {
                ir->_call(args_count.top() + no_comma.top());
                args_count.pop();
                no_comma.pop();
                if (args_count.empty()) is_call = false;
                break;
            }
            case Token::Type::Not: {
                ir->_not();
                break;
            }
            case Token::Type::And: {
                ir->_and();
                break;
            }
            case Token::Type::Or: {
                ir->_or();
                break;
            }
            case Token::Type::Equals: {
                ir->_eq();
                break;
            }
            case Token::Type::Lesser: {
                ir->_lt();
                break;
            }
            case Token::Type::LeEquals: {
                ir->_le();
                break;
            }
            case Token::Type::Greater: {
                ir->_gt();
                break;
            }
            case Token::Type::GrEquals: {
                ir->_ge();
                break;
            }
            case Token::Type::NotEquals: {
                ir->_ne();
                break;
            }
            case Token::Type::UnaryPlus: {
                ir->_promote();
                break;
            }
            case Token::Type::UnaryMinus: {
                ir->_negate();
                break;
            }
            case Token::Type::Comma: {
                if (args_count.empty()) {
                    log->set_snippet(token.snippet);
                    SYNTAXERROR("comma out of array, class or function");
                    return ERROR_IDX;
                }
                ++args_count.top();
                no_comma.top() = false;
                break;
            }
            default: {
                log->set_snippet(token.snippet);
                SYNTAXERROR("[INTERNAL] unknown token with type '%s' in expression", token.type_str());
                return ERROR_IDX;
            }
        }

        //printf("%zu = '%s': %s (values=(%zu, %zu/%s))\n", i, token.lexeme.c_str(), token.type_str(), labels.size(), 
        //        labels.empty() ? 0 : labels.back().first, labels.empty() ? 0 : labels.back().second.c_str());

        return i;
    };

    // For handling different types of tokens
    for (size_t i = 0; i < out.size(); ++i) {
        if (token.is_operand()) {
            ++pop_count;
        } else if (token.is_expr() && !token.is_special()) {
            --pop_count;
        }

        i = transpile(i, out[i]);
        if (i == ERROR_IDX) return ERROR_IDX;

        last = out[i];
    }

    if (has_equal) {
        ir->_refset(-(int)(eq_count + 1));
        --pop_count;
    }

    --pop_count;
    if (pop_count >= 1) {
        //ir->_popn(pop_count);
    } else {
        //ir->_pop();
    }

    // TODO: only make it pop the values if there is an ; operator, else printing the remainder like lua

    return i;
}

/* -=- Token management -=- */
llama::Token llama::Analyser::seek_token(size_t pos) {
    // Does the exact same as the seek() function but for tokens
    if (pos >= lex->tokens.size()) return Token(Token::Type::Unknown);
    return lex->tokens[pos];
}

/* -=- Formatters -=- */
void llama::Analyser::dump() {
    
}