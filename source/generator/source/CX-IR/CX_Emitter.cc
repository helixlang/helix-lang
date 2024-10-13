
//===------------------------------------------ C++ ------------------------------------------====//
//                                                                                                //
//  Part of the Helix Project, under the Attribution 4.0 International license (CC BY 4.0).       //
//  You are allowed to use, modify, redistribute, and create derivative works, even for           //
//  commercial purposes, provided that you give appropriate credit, and indicate if changes       //
//   were made. For more information, please visit: https://creativecommons.org/licenses/by/4.0/  //
//                                                                                                //
//  SPDX-License-Identifier: CC-BY-4.0                                                            //
//  Copyright (c) 2024 (CC BY 4.0)                                                                //
//                                                                                                //
//====----------------------------------------------------------------------------------------====//

#include <cstdio>
#include <ctime>
#include <memory>
#include <stdexcept>

#include "generator/include/CX-IR/CXIR.hh"
#include "neo-pprint/include/hxpprint.hh"
#include "parser/ast/include/config/AST_config.def"
#include "parser/ast/include/nodes/AST_declarations.hh"
#include "parser/ast/include/nodes/AST_expressions.hh"
#include "parser/ast/include/nodes/AST_statements.hh"
#include "parser/ast/include/private/AST_generate.hh"
#include "parser/ast/include/private/base/AST_base.hh"
#include "parser/ast/include/types/AST_modifiers.hh"
#include "parser/ast/include/types/AST_types.hh"
#include "token/include/private/Token_base.hh"
#include "token/include/private/Token_generate.hh"

#define CXIR_NOT_IMPLEMENTED throw std::runtime_error(GET_DEBUG_INFO + "Not implemented yet")
#define ADD_TOKEN(token) tokens.push_back(std::make_unique<CX_Token>(cxir_tokens::token))
#define ADD_TOKEN_AS_VALUE(token, value) \
    tokens.push_back(std::make_unique<CX_Token>(cxir_tokens::token, value))
#define ADD_TOKEN_AS_TOKEN(token, token_value) \
    tokens.push_back(std::make_unique<CX_Token>(token_value, cxir_tokens::token))

#define ADD_NODE_PARAM(param) ADD_PARAM(node.param)
#define ADD_PARAM(param) param->accept(*this)

// This macro will not add a separator after the last element.
#define SEP(args, sep)                                  \
    if (!node.args.empty()) {                           \
        ADD_NODE_PARAM(args[0]);                        \
        for (size_t i = 1; i < node.args.size(); ++i) { \
            sep;                                        \
            ADD_NODE_PARAM(args[i]);                    \
        }                                               \
    }

// This macro always adds the separator, even after the last element.
#define SEP_TRAILING(args, sep)                         \
    if (!node.args.empty())                             \
        for (size_t i = 0; i < node.args.size(); ++i) { \
            ADD_NODE_PARAM(args[i]);                    \
            sep                                         \
        }

#define UNLESS_NULL(nullable_val) if (node.nullable_val != nullptr)

#define ADD_ALL_PARAMS(params)         \
    for (const auto &param : params) { \
        ADD_PARAM(param);              \
    }

#define ADD_ALL_NODE_PARAMS(params) ADD_ALL_PARAMS(node.params)

#define COMMA_SEP(args) SEP(args, ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, ",");)
#define SCOPE_SEP(args) SEP(args, ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, "::");)

#define DELIMIT(L, R, ...) \
    ADD_TOKEN(L);          \
    __VA_ARGS__            \
    ADD_TOKEN(R)

#define PAREN_DELIMIT(...) DELIMIT(CXX_LPAREN, CXX_RPAREN, __VA_ARGS__)
#define BRACE_DELIMIT(...) DELIMIT(CXX_LBRACE, CXX_RBRACE, __VA_ARGS__)
#define BRACKET_DELIMIT(...) DELIMIT(CXX_LBRACKET, CXX_RBRACKET, __VA_ARGS__)
#define ANGLE_DELIMIT(...) DELIMIT(CXX_LESS, CXX_GREATER, __VA_ARGS__)

CX_VISIT_IMPL(LiteralExpr) {
    tokens.push_back(std::make_unique<CX_Token>(node.value, cxir_tokens::CXX_CORE_LITERAL));
}

CX_VISIT_IMPL(LetDecl) {

    // for (auto &var : node.modifiers.) {}

    ADD_ALL_NODE_PARAMS(vars);
}

CX_VISIT_IMPL(BinaryExpr) {
    // -> '(' lhs op  rhs ')'
    // FIXME: are the parens necessary?
    // ADD_TOKEN(CXX_LPAREN);
    ADD_NODE_PARAM(lhs);
    ADD_TOKEN_AS_TOKEN(CXX_CORE_OPERATOR, node.op);
    ADD_NODE_PARAM(rhs);
    // ADD_TOKEN(CXX_RPAREN);
}

CX_VISIT_IMPL(UnaryExpr) {
    // -> '(' op '(' opd ')' ')' | '(' opd ')'

    PAREN_DELIMIT(if (node.op.token_kind() != token::PUNCTUATION_QUESTION_MARK)
                      ADD_TOKEN_AS_TOKEN(CXX_CORE_OPERATOR, node.op);

                  PAREN_DELIMIT(ADD_NODE_PARAM(opd);););
}

CX_VISIT_IMPL(IdentExpr) { ADD_TOKEN_AS_TOKEN(CXX_CORE_IDENTIFIER, node.name); }

CX_VISIT_IMPL(NamedArgumentExpr) {
    // -> name '=' value
    ADD_NODE_PARAM(name);
    ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, "=");
    ADD_NODE_PARAM(value);
}

CX_VISIT_IMPL(ArgumentExpr) { ADD_NODE_PARAM(value); }

CX_VISIT_IMPL(ArgumentListExpr) {

    // -> '(' arg (',' arg)* ')'
    PAREN_DELIMIT(COMMA_SEP(args););
}

CX_VISIT_IMPL(GenericInvokeExpr) { ANGLE_DELIMIT(COMMA_SEP(args);); }

CX_VISIT_IMPL(GenericInvokePathExpr) {
    // := E GenericInvokeExpr
    ADD_NODE_PARAM(generic);  // TODO: WT is this
}

CX_VISIT_IMPL(ScopePathExpr) {
    // -> path ('::' path)*

    for (const parser::ast::NodeT<parser::ast::node::IdentExpr> &ident : node.path) {
        ident->accept(*this);
        ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, "::");
    }

    ADD_NODE_PARAM(access);
}

CX_VISIT_IMPL(DotPathExpr) {
    // -> path '.' path
    ADD_NODE_PARAM(lhs);
    ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, ".");
    ADD_NODE_PARAM(rhs);
}

CX_VISIT_IMPL(ArrayAccessExpr) {
    // -> array '[' index ']'
    ADD_NODE_PARAM(lhs);
    ADD_TOKEN(CXX_LBRACKET);
    ADD_NODE_PARAM(rhs);
    ADD_TOKEN(CXX_RBRACKET);
}

CX_VISIT_IMPL(PathExpr) {  // TODO
    ADD_NODE_PARAM(path);
}

CX_VISIT_IMPL(FunctionCallExpr) {
    // path
    // generic
    // args

    ADD_NODE_PARAM(path);

    UNLESS_NULL(generic) { ADD_NODE_PARAM(generic); };
    ADD_NODE_PARAM(args);
}

CX_VISIT_IMPL(ArrayLiteralExpr) { BRACE_DELIMIT(COMMA_SEP(values);); }

CX_VISIT_IMPL(TupleLiteralExpr) { BRACE_DELIMIT(COMMA_SEP(values);); }
/// TODO: is this even imlpementad in the current stage?
CX_VISIT_IMPL(SetLiteralExpr) { BRACE_DELIMIT(COMMA_SEP(values);); }

CX_VISIT_IMPL(MapPairExpr) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(MapLiteralExpr) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(ObjInitExpr) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(LambdaExpr) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(TernaryExpr) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(ParenthesizedExpr) { PAREN_DELIMIT(ADD_NODE_PARAM(value);); }

CX_VISIT_IMPL(CastExpr) {

    // TODO: WHAT KIND OF CAST???

    PAREN_DELIMIT(PAREN_DELIMIT(ADD_NODE_PARAM(type);); ADD_NODE_PARAM(value););
}

// := E ('has' | 'derives') E
CX_VISIT_IMPL(InstOfExpr) {
    switch (node.op) {
        case parser::ast::node::InstOfExpr::InstanceType::Derives:

            ADD_NODE_PARAM(type);

            ADD_NODE_PARAM(value);

        case parser::ast::node::InstOfExpr::InstanceType::In:
            // In flip flops the type and value
            ADD_NODE_PARAM(value);

            ADD_NODE_PARAM(type);
    }
}

CX_VISIT_IMPL(AsyncThreading) {

    switch (node.type) {
        case parser::ast::node::AsyncThreading::AsyncThreadingType::Await:
            ADD_TOKEN(CXX_CO_AWAIT);
            ADD_NODE_PARAM(value);
            break;
        case parser::ast::node::AsyncThreading::AsyncThreadingType::Spawn:
        case parser::ast::node::AsyncThreading::AsyncThreadingType::Thread:
        case parser::ast::node::AsyncThreading::AsyncThreadingType::Other:
            CXIR_NOT_IMPLEMENTED;
    }
}

CX_VISIT_IMPL(Type) {  // TODO Modifiers

    ADD_NODE_PARAM(value);
    // TODO: make *null into null_ptr

    UNLESS_NULL(generics) ADD_NODE_PARAM(generics);
}

CX_VISIT_IMPL(NamedVarSpecifier) {
    // (type | auto) name

    UNLESS_NULL(type) { ADD_NODE_PARAM(type); }
    else {
        ADD_TOKEN(CXX_AUTO);
    }

    ADD_NODE_PARAM(path);
}

CX_VISIT_IMPL(NamedVarSpecifierList) { COMMA_SEP(vars); }

CX_VISIT_IMPL(ForPyStatementCore) {
    // := NamedVarSpecifier 'in 'expr' Suite

    ADD_TOKEN(CXX_LPAREN);

    ADD_NODE_PARAM(vars);

    ADD_TOKEN(CXX_COLON);

    ADD_NODE_PARAM(range);

    ADD_TOKEN(CXX_RPAREN);

    ADD_NODE_PARAM(body);
}

CX_VISIT_IMPL(ForCStatementCore) {
    PAREN_DELIMIT(                  //
        ADD_NODE_PARAM(init);       //
        ADD_TOKEN(CXX_SEMICOLON);   //
        ADD_NODE_PARAM(condition);  //
        ADD_TOKEN(CXX_SEMICOLON);   //
        ADD_NODE_PARAM(update);     //

    );
    ADD_NODE_PARAM(body);
}

CX_VISIT_IMPL(ForState) {
    // := 'for' (ForPyStatementCore | ForCStatementCore)

    ADD_TOKEN(CXX_FOR);

    ADD_NODE_PARAM(core);
}

CX_VISIT_IMPL(WhileState) {
    // := 'while' expr Suite

    ADD_TOKEN(CXX_WHILE);

    PAREN_DELIMIT(                  //
        ADD_NODE_PARAM(condition);  //
    );
    ADD_NODE_PARAM(body);
}

CX_VISIT_IMPL(ElseState) {

    ADD_TOKEN(CXX_ELSE);

    if (node.type != parser::ast::node::ElseState::ElseType::Else) {
        ADD_TOKEN(CXX_IF);
        PAREN_DELIMIT(  //
            if (node.type != parser::ast::node::ElseState::ElseType::ElseUnless) {
                ADD_TOKEN(CXX_EXCLAMATION);
            }

            PAREN_DELIMIT(                  //
                ADD_NODE_PARAM(condition);  //
            );                              //
        );
    }
    ADD_NODE_PARAM(body);
}

CX_VISIT_IMPL(IfState) {
    ADD_TOKEN(CXX_IF);

    if (node.type == parser::ast::node::IfState::IfType::Unless) {  //
        PAREN_DELIMIT(                                              //
            ADD_TOKEN(CXX_EXCLAMATION); PAREN_DELIMIT(              //
                ADD_NODE_PARAM(condition);                          //
            );                                                      //
        );                                                          //
    } else {
        PAREN_DELIMIT(                  //
            ADD_NODE_PARAM(condition);  //
        );                              //
    }

    ADD_NODE_PARAM(body);

    ADD_ALL_NODE_PARAMS(else_body);
}

CX_VISIT_IMPL(SwitchCaseState) {

    switch (node.type) {

        case parser::ast::node::SwitchCaseState::CaseType::Case:
            ADD_TOKEN(CXX_CASE);
            ADD_NODE_PARAM(condition);

        case parser::ast::node::SwitchCaseState::CaseType::Fallthrough:
            ADD_TOKEN(CXX_CASE);

            ADD_NODE_PARAM(condition);
            ADD_TOKEN(CXX_COLON);

            BRACKET_DELIMIT(                                                 //
                BRACKET_DELIMIT(                                             //
                    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "fallthrough");  //
                );                                                           //
            );                                                               //

            break;

        case parser::ast::node::SwitchCaseState::CaseType::Default:
            ADD_TOKEN(CXX_DEFAULT);
    }
}

CX_VISIT_IMPL(SwitchState) {
    ADD_TOKEN(CXX_SWITCH);

    PAREN_DELIMIT(                  //
        ADD_NODE_PARAM(condition);  //
    );

    BRACE_DELIMIT(                   //
        ADD_ALL_NODE_PARAMS(cases);  //
    );
}

CX_VISIT_IMPL(YieldState) {
    ADD_TOKEN(CXX_CO_YIELD);
    ADD_NODE_PARAM(value);
    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(DeleteState) {
    ADD_TOKEN(CXX_DELETE);
    ADD_NODE_PARAM(value);
    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(AliasState) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(SingleImportState) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(MultiImportState) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(ImportState) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(ReturnState) {
    ADD_TOKEN(CXX_RETURN);  // TODO co_return for generator contexts?
    ADD_NODE_PARAM(value);
    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(BreakState) {
    ADD_TOKEN(CXX_BREAK);
    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(BlockState) {
    // -> (statement ';')*
    SEP_TRAILING(body, ADD_TOKEN(CXX_SEMICOLON););
}

CX_VISIT_IMPL(SuiteState) {
    // -> '{' body '}'
    BRACE_DELIMIT(             //
        ADD_NODE_PARAM(body);  //
    );
}

CX_VISIT_IMPL(ContinueState) { ADD_TOKEN(CXX_CONTINUE); }

CX_VISIT_IMPL(CatchState) {
    ADD_TOKEN(CXX_CATCH);
    PAREN_DELIMIT(                    //
        ADD_NODE_PARAM(catch_state);  //
    );                                //
    ADD_NODE_PARAM(body);
}

CX_VISIT_IMPL(FinallyState) {
    // TODO: this needs to be placed before return, so, the code gen needs to be statefull here...
    // for now it will just put the
    // https://stackoverflow.com/questions/33050620/golang-style-defer-in-c
    // for try catch this would have to be placed before the try block
    // shared_ptr<void>_(nullptr, [] { cout << ", World!"; });
    // ADD_TOKEN_AS_TOKEN(CXX_CORE_IDENTIFIER, "shared_ptr");
    // ANGLE_DELIMIT(ADD_TOKEN(CXX_VOID););
    CXIR_NOT_IMPLEMENTED;
}

CX_VISIT_IMPL(TryState) {

    // Is this nullable?
    UNLESS_NULL(finally_state) {        //
        ADD_NODE_PARAM(finally_state);  //
    }

    ADD_TOKEN(CXX_TRY);
    ADD_NODE_PARAM(body);
}

CX_VISIT_IMPL(PanicState) {
    ADD_TOKEN(CXX_THROW);
    ADD_NODE_PARAM(expr);
    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(ExprState) {
    // -> expr ';'
    ADD_NODE_PARAM(value);
    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(RequiresParamDecl) {
    if (node.is_const) {
        UNLESS_NULL(var) {
            PARSE_ERROR(node.var->path->name, "Const requires a type");  //
            return;                                                      //
        };
    }

    UNLESS_NULL(var) {              //
        ADD_NODE_PARAM(var->type);  //
    }
    else {
        ADD_TOKEN(CXX_TYPENAME);  // template <typename
    }

    ADD_NODE_PARAM(var);

    UNLESS_NULL(value) {                             //
        ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, "=");  //
        ADD_NODE_PARAM(value);                       //
    };
}

CX_VISIT_IMPL(RequiresParamList) {  // -> (param (',' param)*)?
    COMMA_SEP(params);
}

CX_VISIT_IMPL(EnumMemberDecl) {
    ADD_NODE_PARAM(name);
    UNLESS_NULL(value) {
        ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, "=");
        ADD_NODE_PARAM(value);
    };
}

CX_VISIT_IMPL(UDTDeriveDecl) {

    switch (node.derives[0].second.type) {
        case parser::ast::AccessSpecifier::Specifier::Internal:
            CXIR_NOT_IMPLEMENTED;  // TODO: ERROR?
            break;
        case parser::ast::AccessSpecifier::Specifier::Public:
            ADD_TOKEN(CXX_PUBLIC);
            break;
        case parser::ast::AccessSpecifier::Specifier::Protected:
            ADD_TOKEN(CXX_PROTECTED);
            break;
        case parser::ast::AccessSpecifier::Specifier::Private:
            ADD_TOKEN(CXX_PRIVATE);
            break;
    }

    ADD_NODE_PARAM(derives[0].first);
    for (size_t i = 1; i < node.derives.size(); ++i) {
        ADD_TOKEN(CXX_COMMA);
        switch (node.derives[i].second.type) {
            case parser::ast::AccessSpecifier::Specifier::Internal:
                CXIR_NOT_IMPLEMENTED;  // TODO: ERROR?
                break;
            case parser::ast::AccessSpecifier::Specifier::Public:
                ADD_TOKEN(CXX_PUBLIC);
                break;
            case parser::ast::AccessSpecifier::Specifier::Protected:
                ADD_TOKEN(CXX_PROTECTED);
                break;
            case parser::ast::AccessSpecifier::Specifier::Private:
                ADD_TOKEN(CXX_PRIVATE);
                break;
        }

        ADD_NODE_PARAM(derives[i].first);
    }
}

CX_VISIT_IMPL(TypeBoundList) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(TypeBoundDecl) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(RequiresDecl) {
    // -> 'template' '<' params '>'
    ADD_TOKEN(CXX_TEMPLATE);

    ANGLE_DELIMIT(               //
        ADD_NODE_PARAM(params);  //
    );
}

CX_VISIT_IMPL(ModuleDecl) {

    if (node.inline_module) {
        ADD_TOKEN(CXX_INLINE);
    }

    ADD_TOKEN(CXX_NAMESPACE);

    ADD_NODE_PARAM(name);

    ADD_NODE_PARAM(body);
}

CX_VISIT_IMPL(StructDecl) {  // TODO: only enums, types, and unions
    // TODO: Modifiers
    UNLESS_NULL(generics) {        //
        ADD_NODE_PARAM(generics);  //
    };
    ADD_TOKEN(CXX_STRUCT);
    ADD_NODE_PARAM(name);

    UNLESS_NULL(derives) {
        ADD_TOKEN(CXX_COLON);
        ADD_NODE_PARAM(derives);  // should be its own generator
    }
    ADD_NODE_PARAM(body);
    ADD_TOKEN(CXX_SEMICOLON);

    // structs can derive?
    // enums should be able to as well... idea?
}

CX_VISIT_IMPL(ConstDecl) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(ClassDecl) {

    UNLESS_NULL(generics) {        //
        ADD_NODE_PARAM(generics);  //
    };

    ADD_TOKEN(CXX_CLASS);
    ADD_NODE_PARAM(name);

    UNLESS_NULL(derives) {
        ADD_TOKEN(CXX_COLON);
        ADD_NODE_PARAM(derives);  // should be its own generator
    }
    ADD_NODE_PARAM(body);
    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(InterDecl) {
    // can have let const eval, type and functions, default impl functions...
    // TODO: Modifiers
    // InterDecl := 'const'? VisDecl? 'interface' E.IdentExpr UDTDeriveDecl? RequiresDecl?
    // S.Suite
    // ADD_NODE_PARAM(generics); // WE need a custom generics impl here as Self is the first generic

    ADD_NODE_PARAM(generics);  // not null, as I (@jrcarl624) went into the ast and made this change
                               // for interfaces
    ADD_TOKEN(CXX_CONCEPT);

    ADD_NODE_PARAM(name);

    ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, "=");
    if (!(node.derives->derives.empty())) {

#define INSERT_AT_FRONT(i)                                      \
    node.derives->derives[i].first->generics->args.insert(      \
        node.derives->derives[i].first->generics->args.begin(), \
        node.generics->params->params.front())

        INSERT_AT_FRONT(0);

        ADD_PARAM(node.derives->derives[0].first);

        for (size_t i = 1; i < node.derives->derives.size(); ++i) {

            // TODO: accses Spes

            INSERT_AT_FRONT(i);

            ADD_PARAM(node.derives->derives[i].first);

            ADD_TOKEN(CXX_AND);
        }

        UNLESS_NULL(body) { ADD_TOKEN(CXX_AND); }
    }

    ADD_NODE_PARAM(body);

    // bool not_added_req = true;  // there is prob a better way to do this...
    // when we have context we will know if there is a function on the interface...

    if (node.body->body) {

        for (size_t i = 0; i < node.body->body->body.size(); ++i) {

            if (node.body->body->body[i]->getNodeName() != "FuncDecl")
                continue;  // TODO: Error? no error?

            parser::ast::NodeT<parser::ast::node::FuncDecl> fn =
                std::static_pointer_cast<parser::ast::node::FuncDecl>(node.body->body->body[i]);

            if (fn->body != nullptr) {
                // TODO: ERROR
            };

            ADD_TOKEN(CXX_REQUIRES);
            PAREN_DELIMIT(                                        //
                ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "Self");  //
                ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "self");  //
                for (auto &param
                     : fn->params) {              //
                    ADD_TOKEN(CXX_COMMA);         //
                    ADD_PARAM(param->var->type);  //
                    ADD_PARAM(param->var->path);  //
                }  //
            );

            // TODO: The only way to derive an interface is to use
            // static_assert(CONCEPT<CLASSNAME>); but this requires that there is context of the
            // interfaces.
            BRACE_DELIMIT(                                            // The outer
                BRACE_DELIMIT(                                        //
                    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "self");  //
                    ADD_TOKEN(CXX_DOT);                               //
                    ADD_PARAM(fn->name);                              //
                    PAREN_DELIMIT(                                    //
                        for (auto &param                              //
                             : fn->params) {                          //
                            ADD_TOKEN(CXX_COMMA);                     //
                            ADD_PARAM(param->var->path);              // The
                        }  //
                    );                         //
                );                             //
                ADD_TOKEN(CXX_PTR_ACC);        //
                if (fn->returns != nullptr) {  //
                    ADD_PARAM(fn->returns);    //
                } else {                       //
                    ADD_TOKEN(CXX_VOID);       //
                }  //
            );
            ADD_TOKEN(CXX_LOGICAL_AND);
        }
        ADD_TOKEN(CXX_TRUE); // This is just to prevent a syntax error, will be removed in the 
        // future
        ADD_TOKEN(CXX_SEMICOLON);
    }
}

CX_VISIT_IMPL(EnumDecl) {

    ADD_TOKEN(CXX_ENUM);
    ADD_NODE_PARAM(name);

    UNLESS_NULL(derives) {        //
        ADD_TOKEN(CXX_COLON);     //
        ADD_NODE_PARAM(derives);  //
    };

    BRACE_DELIMIT(           //
        COMMA_SEP(members);  //
    );

    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(TypeDecl) {
    // TODO: vis, as there is no way to make a type priv if its not on a class
    ADD_NODE_PARAM(generics);
    ADD_TOKEN(CXX_USING);
    ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, "=");
    ADD_NODE_PARAM(value);
}

CX_VISIT_IMPL(FuncDecl) {
    UNLESS_NULL(generics) {  //
        ADD_NODE_PARAM(generics);
    };

    UNLESS_NULL(returns) {  //
        ADD_NODE_PARAM(returns);
    }
    else {
        ADD_TOKEN(CXX_VOID);
    }

    // if (node.name == nullptr) {
    //     print("error");
    //     throw std::runtime_error("This is bad");
    // }

    ADD_NODE_PARAM(name);
    PAREN_DELIMIT(          //
        COMMA_SEP(params);  //
    );

    ADD_NODE_PARAM(body);
}

CX_VISIT_IMPL(VarDecl) {
    // UNLESS_NULL(var->type, ADD_PARAM(node.var->type);) else { ADD_TOKEN(CXX_AUTO); }

    ADD_NODE_PARAM(var);

    UNLESS_NULL(value) {
        ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, "=");
        ADD_NODE_PARAM(value);
    }
}

CX_VISIT_IMPL(FFIDecl) {
    if (node.name->value.value() != "\"c++\"") {
        throw std::runtime_error("Only C++ is supported at the moment");
    }

    ADD_TOKEN(CXX_PP_INCLUDE);

    if (node.value->getNodeType() == parser::ast::node::nodes::SingleImportState) {
        ADD_TOKEN_AS_VALUE(
            CXX_CORE_LITERAL,
            std::static_pointer_cast<parser::ast::node::LiteralExpr>(
                std::static_pointer_cast<parser::ast::node::SingleImportState>(node.value)->path)
                ->value.value());
    } else {
        throw std::runtime_error("Only string literals are supported at the moment");
    }
}

CX_VISIT_IMPL(OpDecl) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(Program) {
    ADD_TOKEN_AS_VALUE(
        CXX_ANNOTATION,
        R"(///*--- Helix ---*
///
///  Part of the Helix Project, under the Attribution 4.0 International license (CC BY 4.0).
///  You are allowed to use, modify, redistribute, and create derivative works, even for
///  commercial purposes, provided that you give appropriate credit, and indicate if changes
///  were made. For more information, please visit: https://creativecommons.org/licenses/by/4.0/
///
///  SPDX-License-Identifier: CC-BY-4.0
///  Copyright (c) 2024 (CC BY 4.0)
///
///  This file was generated by the Helix compiler, do not modify it directly.
///  Generated on: )" +
            std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())) +
            R"( [unix-timestamp]
///
///*--- Helix ---*
)");

    for (const auto &child : node.children) {
        child->accept(*this);
    }
}