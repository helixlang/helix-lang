///--- The Helix Project ------------------------------------------------------------------------///
///                                                                                              ///
///   Part of the Helix Project, under the Attribution 4.0 International license (CC BY 4.0).    ///
///   You are allowed to use, modify, redistribute, and create derivative works, even for        ///
///   commercial purposes, provided that you give appropriate credit, and indicate if changes    ///
///   were made.                                                                                 ///
///                                                                                              ///
///   For more information on the license terms and requirements, please visit:                  ///
///     https://creativecommons.org/licenses/by/4.0/                                             ///
///                                                                                              ///
///   SPDX-License-Identifier: CC-BY-4.0                                                         ///
///   Copyright (c) 2024 The Helix Project (CC BY 4.0)                                           ///
///                                                                                              ///
///-------------------------------------------------------------------------------------- C++ ---///

#include <algorithm>
#include <chrono>
#include <concepts>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "generator/include/CX-IR/CXIR.hh"
#include "generator/include/config/Gen_config.def"
#include "neo-panic/include/error.hh"
#include "neo-pprint/include/hxpprint.hh"
#include "parser/ast/include/AST.hh"
#include "parser/ast/include/config/AST_config.def"
#include "parser/ast/include/nodes/AST_declarations.hh"
#include "parser/ast/include/nodes/AST_expressions.hh"
#include "parser/ast/include/nodes/AST_statements.hh"
#include "parser/ast/include/private/AST_generate.hh"
#include "parser/ast/include/private/base/AST_base_expression.hh"
#include "parser/ast/include/types/AST_modifiers.hh"
#include "parser/ast/include/types/AST_types.hh"
#include "parser/preprocessor/include/private/utils.hh"
#include "token/include/Token.hh"
#include "token/include/config/Token_config.def"
#include "token/include/private/Token_base.hh"
#include "token/include/private/Token_generate.hh"

enum class OperatorType { UnaryPrefix, UnaryPostfix, Binary, Array, Call, None };

std::string generate_unique_name() {
    auto now    = std::chrono::system_clock::now();
    auto epoch  = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

    std::random_device              rd;
    std::mt19937                    gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    int                             randomComponent = dis(gen);

    std::ostringstream uniqueName;
    uniqueName << "$_" << millis << "_" << randomComponent;

    return uniqueName.str();
}

std::pair<bool, bool> contains_self_static(const __AST_N::NodeT<__AST_NODE::FuncDecl> &func_decl) {
    std::pair<bool, bool>               found_self_static = {false, false};
    __AST_N::NodeT<__AST_NODE::VarDecl> self_arg          = nullptr;

    if (!func_decl->params.empty() && func_decl->params[0] != nullptr) {
        self_arg = func_decl->params[0];
        if (self_arg->var->path->name.value() == "self") {
            if (self_arg->var->type != nullptr || self_arg->value != nullptr) {
                throw error::Panic(
                    error::CodeError{.pof = &self_arg->var->path->name, .err_code = 0.3006});
            }
            found_self_static.first = true;  // found 'self'
        }
    }

    if (func_decl->modifiers.contains(token::tokens::KEYWORD_STATIC)) {
        found_self_static.second = true;  // found 'static'
    }

    return found_self_static;
}

std::pair<bool, bool> contains_self_static(const __AST_N::NodeT<__AST_NODE::OpDecl> &op_decl) {
    auto func_decl       = op_decl->func;
    func_decl->modifiers = op_decl->modifiers;

    return contains_self_static(func_decl);
}

void handle_static_self_fn_decl(__AST_N::NodeT<__AST_NODE::FuncDecl> &func_decl,
                                token::Token                         &pof) {
    auto [has_self, has_static] = contains_self_static(func_decl);

    if (!has_static) {
        if (!has_self) {
            error::Panic(error::CodeError{.pof = &pof, .err_code = 0.3004});

            func_decl->modifiers.add(__AST_N::FunctionSpecifier(
                token::Token(token::tokens::KEYWORD_STATIC, "helix_internal.file")));
        }
    } else if (has_self) {
        throw error::Panic(error::CodeError{.pof = &pof, .err_code = 0.3005});
    }

    if (has_self) {
        func_decl->params.erase(func_decl->params.begin());
    }
}

void handle_static_self_fn_decl(__AST_N::NodeT<__AST_NODE::OpDecl> &op_decl, token::Token &pof) {
    auto &func_decl      = op_decl->func;
    func_decl->modifiers = op_decl->modifiers;

    handle_static_self_fn_decl(func_decl, pof);
}

template <typename T>
concept HasVisiblityModifier = requires(T t) {
    { t->vis } -> std::same_as<__AST_N::Modifiers>;
};

void add_public(generator::CXIR::CXIR *self) {
    self->append(generator::CXIR::cxir_tokens::CXX_PUBLIC);
    self->append(generator::CXIR::cxir_tokens::CXX_COLON);
}

void add_protected(generator::CXIR::CXIR *self) {
    self->append(generator::CXIR::cxir_tokens::CXX_PROTECTED);
    self->append(generator::CXIR::cxir_tokens::CXX_COLON);
}

void add_private(generator::CXIR::CXIR *self) {
    self->append(generator::CXIR::cxir_tokens::CXX_PRIVATE);
    self->append(generator::CXIR::cxir_tokens::CXX_COLON);
}

void add_visibility(generator::CXIR::CXIR                      *self,
                    const __AST_N::NodeT<__AST_NODE::FuncDecl> &func_decl) {
    if (func_decl->modifiers.contains(token::tokens::KEYWORD_PROTECTED)) {
        add_protected(self);
    } else if (func_decl->modifiers.contains(token::tokens::KEYWORD_PRIVATE)) {
        add_private(self);
    } else {
        add_public(self);
    }
}

void add_visibility(generator::CXIR::CXIR                    *self,
                    const __AST_N::NodeT<__AST_NODE::OpDecl> &op_decl) {
    if (op_decl->modifiers.contains(token::tokens::KEYWORD_PROTECTED)) {
        add_protected(self);
    } else if (op_decl->modifiers.contains(token::tokens::KEYWORD_PRIVATE)) {
        add_private(self);
    } else {
        add_public(self);
    }
}

template <typename T>
void add_visibility(generator::CXIR::CXIR *self, const T &decl) {
    if constexpr (HasVisiblityModifier<T>) {
        if (decl->vis.contains(token::tokens::KEYWORD_PROTECTED)) {
            add_protected(self);
        } else if (decl->vis.contains(token::tokens::KEYWORD_PRIVATE)) {
            add_private(self);
        } else {
            add_public(self);
        }
    }
}

OperatorType determine_operator_type(const __AST_N::NodeT<__AST_NODE::OpDecl> &op_decl) {
    // check for self and static context
    auto [is_self, is_static] = contains_self_static(op_decl);

    // helper to detect array operators
    auto is_array_operator = [&]() -> bool {
        return op_decl->op.size() == 2 && op_decl->op[0] == __TOKEN_N::PUNCTUATION_OPEN_BRACKET &&
               op_decl->op[1] == __TOKEN_N::PUNCTUATION_CLOSE_BRACKET;
    };

    // helper to detect call operators
    auto is_call_operator = [&]() -> bool {
        return op_decl->op.size() == 2 && op_decl->op[0] == __TOKEN_N::PUNCTUATION_OPEN_PAREN &&
               op_decl->op[1] == __TOKEN_N::PUNCTUATION_CLOSE_PAREN;
    };

    // distinguish unary prefix vs unary postfix
    auto is_unary_postfix = [&]() -> bool {
        return op_decl->func->params.size() == 1 &&
               (op_decl->op[0] == __TOKEN_N::tokens::OPERATOR_R_INC ||
                op_decl->op[0] == __TOKEN_N::tokens::OPERATOR_R_DEC);
    };

    // handle member operators
    if (is_self) {
        if (is_call_operator()) {
            return OperatorType::Call;  // call operator `()`
        }

        if (op_decl->func->params.size() == 2) {  // (self, ...)
            if (is_array_operator()) {
                return OperatorType::Array;  // array operator `[]`
            }

            return OperatorType::Binary;  // binary operator
        }

        if (op_decl->func->params.size() == 1) {  // (self)
            if (is_unary_postfix()) {
                return OperatorType::UnaryPostfix;  // unary postfix operator `x++` or `x--`
            }

            return OperatorType::UnaryPrefix;  // unary prefix operator
        }

        if (op_decl->func->params.size() > 2) {  // (self, ..., ...)
            if (is_call_operator()) {
                return OperatorType::Array;  // array operator `[]`
            }

            return OperatorType::None;
        }
    }

    // handle static operators (free functions)
    if (is_static) {
        if (is_call_operator()) {
            return OperatorType::None;  // call operator `()`
        }

        /// check if the operator is a member only operator
        if (is_array_operator() || is_call_operator() ||
            (op_decl->op.size() > 0 &&
             (op_decl->op.back() == __TOKEN_N::tokens::OPERATOR_ARROW ||
              op_decl->op.back() == __TOKEN_N::tokens::PUNCTUATION_COMMA))) {
            return OperatorType::None;
        }

        if (op_decl->func->params.empty() || op_decl->func->params.size() == 0) {  // (...)
            if (is_call_operator()) {
                return OperatorType::Call;  // call operator `()`
            }

            return OperatorType::None;
        }

        if (op_decl->func->params.size() == 1) {  // (...)
            if (is_unary_postfix()) {
                return OperatorType::UnaryPostfix;  // unary postfix operator `x++` or `x--`
            }

            return OperatorType::UnaryPrefix;  // unary prefix operator
        }

        if (op_decl->func->params.size() == 2) {  // (..., ...)
            return OperatorType::Binary;          // binary operator
        }

        if (op_decl->func->params.size() > 2) {  // (..., ..., ...)
            return OperatorType::Call;
        }
    }

    return OperatorType::None;  // default case: no valid operator type
}

CX_VISIT_IMPL(LiteralExpr) {
    if (node.contains_format_args) {
        // helix::std::format_string(node.value, (format_arg)...)
        ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "helix");
        ADD_TOKEN(CXX_SCOPE_RESOLUTION);
        ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "std");
        ADD_TOKEN(CXX_SCOPE_RESOLUTION);
        ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "format_string");
        ADD_TOKEN(CXX_LPAREN);
        ADD_TOKEN_AS_TOKEN(CXX_CORE_LITERAL, node.value);
        ADD_TOKEN(CXX_COMMA);

        for (auto &format_spec : node.format_args) {
            PAREN_DELIMIT(format_spec->accept(*this););
            ADD_TOKEN(CXX_COMMA);
        }

        if (node.format_args.size() > 0) {
            tokens.pop_back();  // remove trailing comma
        }

        ADD_TOKEN(CXX_RPAREN);

        return;
    }

    ADD_TOKEN_AS_TOKEN(CXX_CORE_LITERAL, node.value);
}

CX_VISIT_IMPL_VA(LetDecl, bool is_in_statement) {

    // for (int i =0; i<node.modifiers.get<__AST_N::TypeSpecifier>().size(); ++i) {

    //     // node.modifiers.

    // }

    // insert all the modifiers at the start
    auto mods = node.modifiers.get<__AST_N::FunctionSpecifier>();

    for (const auto &mod : mods) {
        if (mod.type == __AST_N::FunctionSpecifier::Specifier::Static) {
            ADD_TOKEN_AS_TOKEN(CXX_STATIC, mod.marker);
        }
    }

    for (const auto &param : node.vars) {
        param->accept(*this);
        if (is_in_statement) {
            ADD_TOKEN(CXX_SEMICOLON);
        }
    };
}

CX_VISIT_IMPL(BinaryExpr) {
    // -> '(' lhs op  rhs ')'
    // FIXME: are the parens necessary?

    /// the one change to this behavior is with the range/range-inclusive operators
    /// we then change the codegen to emit either range(lhs, rhs) or range(lhs, rhs++)

    if (node.op.token_kind() == token::OPERATOR_RANGE || node.op.token_kind() == token::OPERATOR_RANGE_INCLUSIVE) {
        ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "range", node.op);
        ADD_TOKEN(CXX_LPAREN);
        ADD_NODE_PARAM(lhs);
        ADD_TOKEN(CXX_COMMA);
        ADD_NODE_PARAM(rhs);
        
        if (node.op.token_kind() == token::OPERATOR_RANGE_INCLUSIVE) {
            ADD_TOKEN_AT_LOC(CXX_INC, node.op);
        }

        ADD_TOKEN(CXX_RPAREN);
        return;
    }

    ADD_NODE_PARAM(lhs);
    ADD_TOKEN_AS_TOKEN(CXX_CORE_OPERATOR, node.op);
    ADD_NODE_PARAM(rhs);
}

CX_VISIT_IMPL(UnaryExpr) {
    // -> '(' op '(' opd ')' ')' | '(' opd ')'

    /// if op = '&' and value is a IdentExpr with value "null" then it is a nullptr
    /// if op = '*' and value is a IdentExpr with value "null" error

    if (node.in_type) {
        if (node.type == __AST_NODE::UnaryExpr::PosType::PostFix) {
            if (node.op.token_kind() != token::PUNCTUATION_QUESTION_MARK) {  // if foo*
                CODEGEN_ERROR(node.op, "type cannot have postfix specifier");
            }

            if (node.op.token_kind() == token::PUNCTUATION_QUESTION_MARK) {
                node.op.get_value() = "";
            }

            return;
        }

        // prefix
        if (node.opd->getNodeType() == __AST_NODE::nodes::Type) [[likely]] {
            auto type = __AST_NODE::Node::as<__AST_NODE::Type>(node.opd);

            if (type->value->getNodeType() == __AST_NODE::nodes::LiteralExpr) [[unlikely]] {
                auto ident = __AST_NODE::Node::as<__AST_NODE::LiteralExpr>(type->value)->value;

                if (ident.value() == "null") [[unlikely]] {
                    CODEGEN_ERROR(ident, "null is not a valid type, use 'void' instead");
                    return;
                }
            }
        }

        switch (node.op.token_kind()) {
            case token::OPERATOR_BITWISE_AND:
            case token::OPERATOR_MUL:
                break;

            default:
                CODEGEN_ERROR(node.op, "invalid specifier for type");
                return;
        }

        ADD_NODE_PARAM(opd);
        ADD_TOKEN_AS_TOKEN(CXX_CORE_OPERATOR, node.op);

        return;
    }

    if (node.opd->getNodeType() == __AST_NODE::nodes::LiteralExpr) {
        auto ident = __AST_NODE::Node::as<__AST_NODE::LiteralExpr>(node.opd)->value;

        if (ident.value() == "null") [[unlikely]] {
            switch (node.op.token_kind()) {
                case token::OPERATOR_BITWISE_AND:
                    break;

                case token::OPERATOR_MUL:
                    CODEGEN_ERROR(node.op, "cannot dereference null");
                    return;

                default:
                    CODEGEN_ERROR(node.op, "invalid specifier for null");
                    return;
            }

            ident.get_value() = "nullptr";
            ADD_TOKEN_AS_TOKEN(CXX_CORE_OPERATOR, ident);

            return;
        }
    }

    PAREN_DELIMIT(if (node.op.token_kind() != token::PUNCTUATION_QUESTION_MARK)
                      ADD_TOKEN_AS_TOKEN(CXX_CORE_OPERATOR, node.op);

                  PAREN_DELIMIT(ADD_NODE_PARAM(opd);););
}

CX_VISIT_IMPL(IdentExpr) {
    // if self then set to (*this)
    if (node.name.value() == "self") {
        ADD_TOKEN(CXX_LPAREN);
        ADD_TOKEN(CXX_ASTERISK);
        ADD_TOKEN(CXX_THIS);
        ADD_TOKEN(CXX_RPAREN);
        return;
    }

    ADD_TOKEN_AS_TOKEN(CXX_CORE_IDENTIFIER, node.name);
}

CX_VISIT_IMPL(NamedArgumentExpr) {
    // -> name '=' value
    ADD_NODE_PARAM(name);
    if (node.value) {
        ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, "=");
        ADD_NODE_PARAM(value);
    }
}

CX_VISIT_IMPL(ArgumentExpr) { ADD_NODE_PARAM(value); }

CX_VISIT_IMPL(ArgumentListExpr) {
    // -> '(' arg (',' arg)* ')'
    PAREN_DELIMIT(COMMA_SEP(args););
}

CX_VISIT_IMPL(GenericInvokeExpr) { ANGLE_DELIMIT(COMMA_SEP(args);); }

CX_VISIT_IMPL_VA(ScopePathExpr, bool access) {
    // -> path ('::' path)*

    if (node.global_scope) {
        ADD_TOKEN(CXX_SCOPE_RESOLUTION);
    }

    for (const __AST_N::NodeT<__AST_NODE::IdentExpr> &ident : node.path) {
        ident->accept(*this);
        ADD_TOKEN(CXX_SCOPE_RESOLUTION);
    }

    if (access) {
        ADD_NODE_PARAM(access);
    }
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
    BRACKET_DELIMIT(          //
        ADD_NODE_PARAM(rhs);  //
    );
}

CX_VISIT_IMPL(PathExpr) { ADD_NODE_PARAM(path); }

CX_VISIT_IMPL(FunctionCallExpr) {
    // path
    // generic
    // args

    size_t depth = 0;

    if (node.path->get_back_name().value() == "__inline_cpp") {
        if (node.args->getNodeType() != __AST_NODE::nodes::ArgumentListExpr ||
            __AST_N::as<__AST_NODE::ArgumentListExpr>(node.args)->args.size() != 1) {
            auto bad_tok = node.path->get_back_name();
            CODEGEN_ERROR(bad_tok, "__inline_cpp requires exactly one argument");
        }

        auto arg = __AST_N::as<__AST_NODE::ArgumentListExpr>(node.args)->args[0];
        if (arg->getNodeType() != __AST_NODE::nodes::ArgumentExpr ||
            __AST_N::as<__AST_NODE::ArgumentExpr>(arg)->value->getNodeType() !=
                __AST_NODE::nodes::LiteralExpr) {
            auto bad_tok = node.path->get_back_name();
            CODEGEN_ERROR(bad_tok, "__inline_cpp requires a string literal argument");
        }

        auto arg_ptr =
            __AST_N::as<__AST_NODE::LiteralExpr>(__AST_N::as<__AST_NODE::ArgumentExpr>(arg)->value);
        if (arg_ptr->contains_format_args) {
            auto bad_tok = node.path->get_back_name();
            CODEGEN_ERROR(bad_tok, "__inline_cpp does not support format arguments");
        }

        auto arg_str = arg_ptr->value;
        arg_str.get_value() =
            arg_str.value().substr(1, arg_str.value().size() - 2);  // remove quotes
        ADD_TOKEN_AS_TOKEN(CXX_INLINE_CODE, arg_str);

        return;
    }

    switch (node.path->type) {
        case __AST_NODE::PathExpr::PathType::Scope: {
            __AST_N::NodeT<__AST_NODE::ScopePathExpr> scope =
                __AST_N::as<__AST_NODE::ScopePathExpr>(node.path->path);
            this->visit(*scope, false);

            if (node.generic && scope->path.size() >= 1) {
                ADD_TOKEN(CXX_TEMPLATE);
            }

            ADD_PARAM(scope->access);
            break;
        }
        case __AST_NODE::PathExpr::PathType::Dot: {
            __AST_N::NodeT<__AST_NODE::DotPathExpr> dot =
                __AST_N::as<__AST_NODE::DotPathExpr>(node.path->path);

            ++depth;
            ADD_PARAM(dot->lhs);
            ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, ".");

            __AST_N::NodeT<> current = dot->rhs;

            while (auto next = std::dynamic_pointer_cast<__AST_NODE::DotPathExpr>(current)) {
                ++depth;

                ADD_PARAM(next->lhs);
                ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, ".");

                current = next->rhs;
            }

            if (node.generic && depth >= 1) {
                ADD_TOKEN(CXX_TEMPLATE);
            }

            ADD_PARAM(current);

            break;
        }
        case __AST_NODE::PathExpr::PathType::Identifier: {
            ADD_NODE_PARAM(path);
            break;
        }
    }

    ADD_NODE_PARAM(generic);
    ADD_NODE_PARAM(args);
}

CX_VISIT_IMPL(ArrayLiteralExpr) { BRACE_DELIMIT(COMMA_SEP(values);); }

CX_VISIT_IMPL(TupleLiteralExpr) {
    if (!node.values.empty() && (node.values.size() > 0) &&
        (node.values[0]->getNodeType() == __AST_NODE::nodes::Type)) {

        ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "tuple");
        ANGLE_DELIMIT(COMMA_SEP(values););

        return;
    }

    BRACE_DELIMIT(COMMA_SEP(values););
}
/// TODO: is this even imlpementad in the current stage?
CX_VISIT_IMPL(SetLiteralExpr) { BRACE_DELIMIT(COMMA_SEP(values);); }

CX_VISIT_IMPL(MapPairExpr) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(MapLiteralExpr) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(ObjInitExpr) {
    if (node.path) {
        ADD_NODE_PARAM(path);
    }

    BRACE_DELIMIT(  //
        if (!node.kwargs.empty()) {
            ADD_NODE_PARAM(kwargs[0]->value);

            for (size_t i = 1; i < node.kwargs.size(); ++i) {
                ADD_TOKEN(CXX_COMMA);
                ADD_NODE_PARAM(kwargs[i]->value);
            }
        }  //
    );
}

CX_VISIT_IMPL(LambdaExpr) { CXIR_NOT_IMPLEMENTED; }

CX_VISIT_IMPL(TernaryExpr) {

    PAREN_DELIMIT(                  //
        ADD_NODE_PARAM(condition);  //
    );
    ADD_TOKEN(CXX_QUESTION);
    ADD_NODE_PARAM(if_true);
    ADD_TOKEN(CXX_COLON);
    ADD_NODE_PARAM(if_false);
}

CX_VISIT_IMPL(ParenthesizedExpr) { PAREN_DELIMIT(ADD_NODE_PARAM(value);); }

CX_VISIT_IMPL(CastExpr) {

    // TODO: WHAT KIND OF CAST???
    // static for now...

    if (node.type->specifiers.contains(token::tokens::KEYWORD_UNSAFE)) {
        ADD_TOKEN(CXX_REINTERPRET_CAST);
    } else {
        ADD_TOKEN(CXX_STATIC_CAST);
    }

    ANGLE_DELIMIT(             //
        ADD_NODE_PARAM(type);  //
    );

    PAREN_DELIMIT(              //
        ADD_NODE_PARAM(value);  //
    );
}

// := E ('in' | 'derives') E
CX_VISIT_IMPL(InstOfExpr) {
    switch (node.op) {
        case __AST_NODE::InstOfExpr::InstanceType::Derives:
            // TODO: make it so it does not require that it is a class
            /// std::is_base_of<A, B>::value
            ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "std");
            ADD_TOKEN(CXX_SCOPE_RESOLUTION);
            ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "is_base_of");
            ANGLE_DELIMIT(              //
                ADD_NODE_PARAM(type);   //
                ADD_TOKEN(CXX_COMMA);   //
                ADD_NODE_PARAM(value);  //
            );
            ADD_TOKEN(CXX_SCOPE_RESOLUTION);
            ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "value");

            break;
        case __AST_NODE::InstOfExpr::InstanceType::Has:
            // we gotta add the value to the generics of type

            // see if we need to have value be a type or an expression based on usage
            // if (node.value->getNodeType() == __AST_NODE::nodes::Type) {
            if (node.type->generics == nullptr || node.type->generics->args.empty()) {
                node.type->generics = std::make_shared<__AST_NODE::GenericInvokeExpr>(node.value);
            } else {
                node.type->generics->args.insert(node.type->generics->args.begin(), node.value);
            }
            // }

            // value 'has' type
            // T has Foo::<int>
            // becomes: Foo<T, int>

            ADD_NODE_PARAM(type);
    }
}

CX_VISIT_IMPL(AsyncThreading) {
    switch (node.type) {
        case __AST_NODE::AsyncThreading::AsyncThreadingType::Await:
            ADD_TOKEN(CXX_CO_AWAIT);
            ADD_NODE_PARAM(value);
            break;
        case __AST_NODE::AsyncThreading::AsyncThreadingType::Spawn:
        case __AST_NODE::AsyncThreading::AsyncThreadingType::Thread:
        case __AST_NODE::AsyncThreading::AsyncThreadingType::Other:
            CXIR_NOT_IMPLEMENTED;
    }
}

CX_VISIT_IMPL(Type) {  // TODO Modifiers
    if (node.specifiers.contains(token::tokens::KEYWORD_YIELD)) {
        auto marker = node.specifiers.get(token::tokens::KEYWORD_YIELD);

        ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "helix", marker);

        ADD_TOKEN(CXX_SCOPE_RESOLUTION);

        ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "generator", marker);
        ANGLE_DELIMIT(ADD_NODE_PARAM(value); ADD_NODE_PARAM(generics););

        return;
    }

    ADD_NODE_PARAM(value);
    ADD_NODE_PARAM(generics);
}

CX_VISIT_IMPL_VA(NamedVarSpecifier, bool omit_t) {
    // (type | auto) name

    if (!omit_t) {
        if (node.type) {
            ADD_NODE_PARAM(type);
        } else {
            ADD_TOKEN(CXX_AUTO);
        }
    }

    ADD_NODE_PARAM(path);
}

CX_VISIT_IMPL_VA(NamedVarSpecifierList, bool omit_t) {
    if (!node.vars.empty()) {
        if (node.vars[0] != nullptr) {
            visit(*node.vars[0], omit_t);
        }
        for (size_t i = 1; i < node.vars.size(); ++i) {
            ADD_TOKEN(CXX_COMMA);
            if (node.vars[i] != nullptr) {
                visit(*node.vars[i], omit_t);
            }
        }
    };
}

CX_VISIT_IMPL(ForPyStatementCore) {
    // := NamedVarSpecifier 'in 'expr' Suite

    ADD_TOKEN(CXX_LPAREN);
    // auto &[a, b]

    if (node.vars->vars.size() > 1) {
        ADD_TOKEN(CXX_AUTO);
        ADD_TOKEN(CXX_AMPERSAND);
        ADD_TOKEN(CXX_LBRACKET);

        if (node.vars != nullptr) {
            visit(*node.vars, true);
        }

        ADD_TOKEN(CXX_RBRACKET);
    } else {
        ADD_NODE_PARAM(vars);
    }

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
    NO_EMIT_FORWARD_DECL;
    // := 'for' (ForPyStatementCore | ForCStatementCore)

    ADD_TOKEN(CXX_FOR);

    ADD_NODE_PARAM(core);
}

CX_VISIT_IMPL(WhileState) {
    NO_EMIT_FORWARD_DECL;
    // := 'while' expr Suite

    ADD_TOKEN(CXX_WHILE);

    PAREN_DELIMIT(                  //
        ADD_NODE_PARAM(condition);  //
    );
    ADD_NODE_PARAM(body);
}

CX_VISIT_IMPL(ElseState) {
    NO_EMIT_FORWARD_DECL;

    ADD_TOKEN(CXX_ELSE);

    if (node.type != __AST_NODE::ElseState::ElseType::Else) {
        ADD_TOKEN(CXX_IF);
        PAREN_DELIMIT(  //
            if (node.type != __AST_NODE::ElseState::ElseType::ElseUnless) {
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
    NO_EMIT_FORWARD_DECL;
    ADD_TOKEN(CXX_IF);

    if (node.type == __AST_NODE::IfState::IfType::Unless) {  //
        PAREN_DELIMIT(                                       //
            ADD_TOKEN(CXX_EXCLAMATION);                      //
            PAREN_DELIMIT(                                   //
                ADD_NODE_PARAM(condition);                   //
            );                                               //
        );                                                   //
    } else {
        PAREN_DELIMIT(                  //
            ADD_NODE_PARAM(condition);  //
        );                              //
    }

    ADD_NODE_PARAM(body);

    ADD_ALL_NODE_PARAMS(else_body);
}

CX_VISIT_IMPL(SwitchCaseState) {
    NO_EMIT_FORWARD_DECL;
    // What are case markers?? TODO: Implement them
    switch (node.type) {

        case __AST_NODE::SwitchCaseState::CaseType::Case:
            ADD_TOKEN(CXX_CASE);
            ADD_NODE_PARAM(condition);
            ADD_TOKEN(CXX_COLON);
            ADD_NODE_PARAM(body);
            break;

        case __AST_NODE::SwitchCaseState::CaseType::Fallthrough:
            ADD_TOKEN(CXX_CASE);
            ADD_NODE_PARAM(condition);
            ADD_TOKEN(CXX_COLON);

            BRACKET_DELIMIT(                                                 //
                BRACKET_DELIMIT(                                             //
                    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "fallthrough");  //
                );                                                           //
            );                                                               //

            break;

        case __AST_NODE::SwitchCaseState::CaseType::Default:
            ADD_TOKEN(CXX_DEFAULT);
            ADD_TOKEN(CXX_COLON);
            ADD_NODE_PARAM(body);
    }
}

CX_VISIT_IMPL(SwitchState) {
    NO_EMIT_FORWARD_DECL;
    ADD_TOKEN(CXX_SWITCH);

    PAREN_DELIMIT(                  //
        ADD_NODE_PARAM(condition);  //
    );

    BRACE_DELIMIT(                   //
        ADD_ALL_NODE_PARAMS(cases);  //
    );
}

CX_VISIT_IMPL(YieldState) {
    NO_EMIT_FORWARD_DECL;
    ADD_TOKEN(CXX_CO_YIELD);
    ADD_NODE_PARAM(value);
    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(DeleteState) {
    NO_EMIT_FORWARD_DECL;
    ADD_TOKEN(CXX_DELETE);
    ADD_NODE_PARAM(value);
    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(ImportState) {
    NO_EMIT_FORWARD_DECL;
    throw std::runtime_error(std::string(colors::fg16::green) + std::string(__FILE__) + ":" +
                             std::to_string(__LINE__) + colors::reset + std::string(" - ") +
                             "This shouldn't be called, should be handled by the Preprocessor not "
                             "CodeGen, something went wrong.");
}
CX_VISIT_IMPL(ImportItems) {
    NO_EMIT_FORWARD_DECL;
    throw std::runtime_error(std::string(colors::fg16::green) + std::string(__FILE__) + ":" +
                             std::to_string(__LINE__) + colors::reset + std::string(" - ") +
                             "This shouldn't be called, should be handled by the Preprocessor not "
                             "CodeGen, something went wrong.");
}
CX_VISIT_IMPL(SingleImport) {
    NO_EMIT_FORWARD_DECL;
    throw std::runtime_error(std::string(colors::fg16::green) + std::string(__FILE__) + ":" +
                             std::to_string(__LINE__) + colors::reset + std::string(" - ") +
                             "This shouldn't be called, should be handled by the Preprocessor not "
                             "CodeGen, something went wrong.");
}
CX_VISIT_IMPL(SpecImport) {
    NO_EMIT_FORWARD_DECL;
    throw std::runtime_error(std::string(colors::fg16::green) + std::string(__FILE__) + ":" +
                             std::to_string(__LINE__) + colors::reset + std::string(" - ") +
                             "This shouldn't be called, should be handled by the Preprocessor not "
                             "CodeGen, something went wrong.");
}
CX_VISIT_IMPL(MultiImportState) {
    NO_EMIT_FORWARD_DECL;
    throw std::runtime_error(std::string(colors::fg16::green) + std::string(__FILE__) + ":" +
                             std::to_string(__LINE__) + colors::reset + std::string(" - ") +
                             "This shouldn't be called, should be handled by the Preprocessor not "
                             "CodeGen, something went wrong.");
}

CX_VISIT_IMPL(ReturnState) {
    NO_EMIT_FORWARD_DECL;
    ADD_TOKEN(CXX_RETURN);  // TODO co_return for generator contexts?
    ADD_NODE_PARAM(value);
    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(BreakState) {
    NO_EMIT_FORWARD_DECL;
    ADD_TOKEN(CXX_BREAK);
    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(BlockState) {
    // -> (statement ';')*
    if (!node.body.empty()) {
        for (const auto &i : node.body) {
            if (i != nullptr) {
                if (i->getNodeType() == __AST_NODE::nodes::LetDecl) {
                    __AST_N::NodeT<__AST_NODE::LetDecl> node = __AST_N::as<__AST_NODE::LetDecl>(i);
                    visit(*node, true);
                } else {
                    i->accept(*this);
                }
            }

            tokens.push_back(std::make_unique<CX_Token>(cxir_tokens::CXX_SEMICOLON));
        }
    }
}

CX_VISIT_IMPL(SuiteState) {
    // -> '{' body '}'
    BRACE_DELIMIT(  //

        if (node.body) { ADD_NODE_PARAM(body); }  //
    );
}
CX_VISIT_IMPL(ContinueState) {
    NO_EMIT_FORWARD_DECL;
    ADD_TOKEN(CXX_CONTINUE);
}

CX_VISIT_IMPL(CatchState) {
    NO_EMIT_FORWARD_DECL;
    ADD_TOKEN(CXX_CATCH);
    PAREN_DELIMIT(                    //
        ADD_NODE_PARAM(catch_state);  //
    );                                //
    ADD_NODE_PARAM(body);
}

CX_VISIT_IMPL(FinallyState) {
    NO_EMIT_FORWARD_DECL;
    // TODO: this needs to be placed before return, so, the code gen needs to be statefull here...
    // for now it will just put the
    // https://stackoverflow.com/questions/33050620/golang-style-defer-in-c

    /// $finally _([&] { free(a); });

    ADD_TOKEN(CXX_SCOPE_RESOLUTION);
    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "helix");
    ADD_TOKEN(CXX_SCOPE_RESOLUTION);
    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "$finally");
    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "_" + generate_unique_name());
    ADD_TOKEN(CXX_LPAREN);
    ADD_TOKEN(CXX_LBRACKET);
    ADD_TOKEN(CXX_AMPERSAND);
    ADD_TOKEN(CXX_RBRACKET);
    ADD_NODE_PARAM(body);
    ADD_TOKEN(CXX_RPAREN);
    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(TryState) {
    NO_EMIT_FORWARD_DECL;

    // Is this nullable?
    if (node.finally_state) {           //
        ADD_NODE_PARAM(finally_state);  //
    }

    ADD_TOKEN(CXX_TRY);
    ADD_NODE_PARAM(body);
}

CX_VISIT_IMPL(PanicState) {
    NO_EMIT_FORWARD_DECL;
    ADD_TOKEN(CXX_THROW);
    ADD_NODE_PARAM(expr);
    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(ExprState) {
    NO_EMIT_FORWARD_DECL;
    // -> expr ';'
    ADD_NODE_PARAM(value);
    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(RequiresParamDecl) {
    if (node.is_const) {
        if (node.var && !node.var->type) {
            CODEGEN_ERROR(node.var->path->name, "const requires a type");
            return;
        };
    }

    if (node.var->type) {           //
        ADD_NODE_PARAM(var->type);  // TODO: verify
    } else {
        ADD_TOKEN(CXX_TYPENAME);  // template <typename
    }

    ADD_NODE_PARAM(var->path);

    if (node.value) {                                //
        ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, "=");  //
        ADD_NODE_PARAM(value);                       //
    };
}

CX_VISIT_IMPL(RequiresParamList) {  // -> (param (',' param)*)?
    COMMA_SEP(params);
}

CX_VISIT_IMPL(EnumMemberDecl) {
    ADD_NODE_PARAM(name);
    if (node.value) {
        ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, "=");
        ADD_NODE_PARAM(value);
    };
}

CX_VISIT_IMPL(UDTDeriveDecl) {

    switch (node.derives[0].second.type) {
        case __AST_N::AccessSpecifier::Specifier::Internal:
            CXIR_NOT_IMPLEMENTED;  // TODO: ERROR?
            break;
        case __AST_N::AccessSpecifier::Specifier::Public:
            ADD_TOKEN(CXX_PUBLIC);
            break;
        case __AST_N::AccessSpecifier::Specifier::Protected:
            ADD_TOKEN(CXX_PROTECTED);
            break;
        case __AST_N::AccessSpecifier::Specifier::Private:
            ADD_TOKEN(CXX_PRIVATE);
            break;
    }

    ADD_NODE_PARAM(derives[0].first);
    for (size_t i = 1; i < node.derives.size(); ++i) {
        ADD_TOKEN(CXX_COMMA);
        switch (node.derives[i].second.type) {
            case __AST_N::AccessSpecifier::Specifier::Internal:
                CXIR_NOT_IMPLEMENTED;  // TODO: ERROR?
                break;
            case __AST_N::AccessSpecifier::Specifier::Public:
                ADD_TOKEN(CXX_PUBLIC);
                break;
            case __AST_N::AccessSpecifier::Specifier::Protected:
                ADD_TOKEN(CXX_PROTECTED);
                break;
            case __AST_N::AccessSpecifier::Specifier::Private:
                ADD_TOKEN(CXX_PRIVATE);
                break;
        }

        ADD_NODE_PARAM(derives[i].first);
    }
}

CX_VISIT_IMPL(TypeBoundList){

    SEP(bounds, ADD_TOKEN(CXX_LOGICAL_AND))

}

CX_VISIT_IMPL(TypeBoundDecl) {
    ADD_NODE_PARAM(bound);
}

CX_VISIT_IMPL(RequiresDecl) {
    // -> 'template' '<' params '>'
    ADD_TOKEN(CXX_TEMPLATE);

    ANGLE_DELIMIT(               //
        ADD_NODE_PARAM(params);  //
    );

    if (node.bounds) {
        ADD_TOKEN(CXX_REQUIRES);
        ADD_NODE_PARAM(bounds);
    }
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
    if (node.generics) {           //
        ADD_NODE_PARAM(generics);  //
    };

    // ADD_TOKEN(CXX_TYPEDEF);

    ADD_TOKEN(CXX_STRUCT);

    ADD_NODE_PARAM(name);

    if (node.derives) {
        ADD_TOKEN(CXX_COLON);
        ADD_NODE_PARAM(derives);  // should be its own generator
    }

    ADD_NODE_PARAM(body);
    ADD_TOKEN(CXX_SEMICOLON);

    // structs can derive?
    // enums should be able to as well... idea?
}

CX_VISIT_IMPL(ConstDecl) {
    for (const auto &param : node.vars) {
        ADD_TOKEN(CXX_CONST);
        param->accept(*this);
    };
}

CX_VISIT_IMPL(ClassDecl) {
    auto add_udt_body = [node](CXIR                                         *self,
                               const __AST_N::NodeT<__AST_NODE::IdentExpr>   name,
                               const __AST_N::NodeT<__AST_NODE::SuiteState> &body) {
        if (body != nullptr) {
            self->append(cxir_tokens::CXX_LBRACE);

            for (auto &child : body->body->body) {
                if (child->getNodeType() == __AST_NODE::nodes::FuncDecl) {
                    auto         func_decl = __AST_N::as<__AST_NODE::FuncDecl>(child);
                    token::Token func_name = func_decl->name->get_back_name();

                    handle_static_self_fn_decl(func_decl, func_name);
                    add_visibility(self, func_decl);

                    if (name != nullptr) {
                        self->visit(*func_decl, func_name.value() == name->name.value());
                    } else {
                        self->visit(*func_decl);
                    }

                } else if (child->getNodeType() == __AST_NODE::nodes::OpDecl) {
                    auto         op_decl = __AST_N::as<__AST_NODE::OpDecl>(child);
                    token::Token op_name;

                    if (op_decl->func->name != nullptr) {
                        op_name = op_decl->func->name->get_back_name();
                    } else {
                        op_name = op_decl->op.back();
                    }

                    handle_static_self_fn_decl(op_decl, op_name);
                    add_visibility(self, op_decl->func);

                    self->visit(*op_decl);

                } else {
                    add_visibility(self, child);
                    child->accept(*self);
                    self->append(cxir_tokens::CXX_SEMICOLON);
                }
            }

            self->append(cxir_tokens::CXX_RBRACE);
        }
    };

    if (node.generics != nullptr) {
        ADD_NODE_PARAM(generics);
    }

    ADD_TOKEN(CXX_CLASS);
    ADD_NODE_PARAM(name);

    if (node.derives) {
        ADD_TOKEN(CXX_COLON);
        ADD_NODE_PARAM(derives);
    }

    if (node.body != nullptr) {
        add_udt_body(this, node.name, node.body);
    }

    ADD_TOKEN(CXX_SEMICOLON);
    
    /// interface support
    /// FIXME: this does not work in the cases where the class takes a generic type
    // if (!node.extends.empty()) {
    //     /// static_assert(extend<class<gens>>, "... must satisfy ... interface");
    //     token::Token loc;

    //     auto add_token = [this, &loc](const cxir_tokens &tok) {
    //         this->append(std::make_unique<CX_Token>(tok, loc));
    //     };

    //     /// class Foo::<T> extends Bar::<T> requires <T> {}
    //     /// static_assert(Bar<Foo<T>, T>, "Foo<T> must satisfy Bar<T> interface");
    //     for (auto &_extend : node.extends) {
    //         auto &extend = std::get<0>(_extend);
    //         loc          = extend->marker;

    //         add_token(cxir_tokens::CXX_STATIC_ASSERT);
    //         add_token(cxir_tokens::CXX_LPAREN);

    //         if (extend->is_fn_ptr) {
    //             PARSE_ERROR(loc, "Function pointers are not allowed in extends");
    //         } else if (extend->nullable) {
    //             PARSE_ERROR(loc, "Nullable types are not allowed in extends");
    //         }

    //         __AST_N::NodeT<__AST_NODE::Type> type_node =
    //             parser::ast::make_node<__AST_NODE::Type>(true);
    //         type_node->value = node.name;

    //         __AST_N::NodeV<__AST_NODE::IdentExpr> args;

    //         if (node.generics) {
    //             for (auto &arg : node.generics->params->params) {
    //                 args.emplace_back(arg->var->path);
    //             }

    //             type_node->generics = parser::ast::make_node<__AST_NODE::GenericInvokeExpr>(
    //                 __AST_N::as<>(args));
    //         }

    //         /// append the class name and generics to extend
    //         if (extend->generics) {
    //             extend->generics->args.insert(extend->generics->args.begin(),
    //                                             __AST_N::as<__AST_NODE::Node>(type_node));
    //         } else {
    //             extend->generics = parser::ast::make_node<__AST_NODE::GenericInvokeExpr>(
    //                 __AST_N::as<__AST_NODE::Node>(type_node));
    //         }

    //         extend->accept(*this);
    //         add_token(cxir_tokens::CXX_COMMA);

    //         this->append(std::make_unique<CX_Token>(
    //             cxir_tokens::CXX_CORE_LITERAL,
    //             "\"" + node.name->name.value() + " must satisfy " + extend->marker.value() +
    //                 " interface\"",
    //             loc));
    //         add_token(cxir_tokens::CXX_RPAREN);
    //         add_token(cxir_tokens::CXX_SEMICOLON);
    //     }
    // }
}

CX_VISIT_IMPL(InterDecl) {
    // can have let const eval, type and functions, default impl functions...
    // TODO: Modifiers
    // InterDecl := 'const'? VisDecl? 'interface' E.IdentExpr UDTDeriveDecl? RequiresDecl?
    // S.Suite
    // ADD_NODE_PARAM(generics); // WE need a custom generics impl here as Self is the first generic

    /// op + fn add(self, other: self) -> self;
    /// { $self + b } -> std::same_as<self>;
    /// { $self.add(b) } -> std::same_as<self>;

    // template <typename self>
    // concept MultiMethod = requires(self t) {
    //     // Check for instance methods
    //     { t.instanceMethod() } -> std::same_as<void>;
    //     { t.getValue() } -> std::same_as<int>;
    //     // check for var
    //     { t.var } -> std::same_as<int>;

    //     // Check for a constructor
    //     self(); // Default constructor

    //     // Check for a static method
    //     { self::staticMethod() } -> std::same_as<void>;
    // };

    /// the only things allwoed in a n itnerface delc:
    /// - LetDecl
    /// - FuncDecl
    /// - OpDecl
    /// - TypeDecl
    /// - ConstDecl

    if (node.name == nullptr) {
        throw std::runtime_error(
            std::string(colors::fg16::green) + std::string(__FILE__) + ":" +
            std::to_string(__LINE__) + colors::reset + std::string(" - ") +
            "Interface Declaration is missing the name param (ub), open an issue on github.");
    }

    auto is_self_t = [&](__AST_N::NodeT<__AST_NODE::Type> &typ) -> bool {
        if (typ->value->getNodeType() == __AST_NODE::nodes::IdentExpr) {
            __AST_N::NodeT<__AST_NODE::IdentExpr> ident =
                __AST_N::as<__AST_NODE::IdentExpr>(typ->value);

            if (ident->name.value() == "self") {
                /// add the self directly
                ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "self", ident->name);
                return true;
            }
        }

        return false;
    };

    token::Token self           = node.name->name;
    std::string  self_parm_name = generate_unique_name();
    std::unordered_map<__AST_N::NodeT<__AST_NODE::VarDecl> *, std::string> type_map;

    auto self_tok = __AST_N::make_node<__AST_NODE::RequiresParamDecl>(
        __AST_N::make_node<__AST_NODE::NamedVarSpecifier>(__AST_N::make_node<__AST_NODE::IdentExpr>(
            __TOKEN_N::Token(self.line_number(),
                             self.column_number(),
                             4,
                             (self.offset() - self.length()) + 4,
                             "self",
                             self.file_name(),
                             "_"))));

    if (node.generics) {  //
        node.generics->params->params.insert(node.generics->params->params.begin(), self_tok);
    } else {
        const_cast<__AST_N::NodeT<__AST_NODE::RequiresDecl> &>(node.generics) =
            __AST_N::make_node<__AST_NODE::RequiresDecl>(
                __AST_N::make_node<__AST_NODE::RequiresParamList>(self_tok));
    }

    for (auto &decl : node.body->body->body) {
        switch (decl->getNodeType()) {
            case __AST_NODE::nodes::FuncDecl: {
                __AST_N::NodeT<__AST_NODE::FuncDecl> func_decl =
                    __AST_N::as<__AST_NODE::FuncDecl>(decl);

                auto [$self, $static] = contains_self_static(func_decl);
                if ($self && $static) {
                    CODEGEN_ERROR(self,
                                  "function is marked static but also takes a self parameter");
                    break;
                }

                if (func_decl->body != nullptr) {
                    CODEGEN_ERROR(func_decl->get_name_t().back(),
                                  "functions have to be forward declarations in an interface.")
                    break;
                }

                if (func_decl->generics != nullptr) {
                    CODEGEN_ERROR(func_decl->get_name_t().back(),
                                  "functions can not have `requires` in an interface, apply them "
                                  "to the interface itself instead.");
                    break;
                }

                if (!func_decl->params.empty()) {
                    bool first = $self;

                    for (auto &param : func_decl->params) {
                        if (first) {
                            first = !first;
                            continue;
                        }

                        type_map[&param] = generate_unique_name();
                    }
                }

                break;
            }

            case __AST_NODE::nodes::OpDecl: {  // WARN: this does not remove the self param from the
                                               //       function decl
                __AST_N::NodeT<__AST_NODE::OpDecl> op_decl = __AST_N::as<__AST_NODE::OpDecl>(decl);

                auto [$self, $static] = contains_self_static(op_decl);
                if ($self && $static) {
                    CODEGEN_ERROR(self,
                                  "function is marked static but also takes a self parameter");
                    break;
                }

                if (op_decl->func->body != nullptr) {
                    CODEGEN_ERROR(op_decl->func->get_name_t().back(),
                                  "functions have to be forward declarations in an interface.")
                    break;
                }

                if (op_decl->func->generics != nullptr) {
                    CODEGEN_ERROR(op_decl->func->get_name_t().back(),
                                  "functions can not have `requires` in an interface, apply them "
                                  "to the interface itself instead.");
                    break;
                }

                if (!op_decl->func->params.empty()) {
                    bool first = $self;

                    for (auto &param : op_decl->func->params) {
                        if (first) {
                            first = !first;
                            continue;
                        }

                        type_map[&param] = generate_unique_name();
                    }
                }

                break;
            }

            /// ------------------------- only validation at this stage ------------------------ ///
            case __AST_NODE::nodes::TypeDecl: {
                __AST_N::NodeT<__AST_NODE::TypeDecl> type_decl =
                    __AST_N::as<__AST_NODE::TypeDecl>(decl);
                CODEGEN_ERROR(type_decl->name->name,
                              "Type definitions are not allowed in interfaces");
                return;
            }
            case __AST_NODE::nodes::LetDecl: {
                __AST_N::NodeT<__AST_NODE::LetDecl> let_decl =
                    __AST_N::as<__AST_NODE::LetDecl>(decl);

                for (auto &var : let_decl->vars) {
                    if (var->value != nullptr) {
                        CODEGEN_ERROR(
                            var->var->path->name,
                            "const decorations in interfaces can not have a default value.");
                        break;
                    }
                }

                break;
            }
            case __AST_NODE::nodes::ConstDecl: {
                __AST_N::NodeT<__AST_NODE::ConstDecl> const_decl =
                    __AST_N::as<__AST_NODE::ConstDecl>(decl);
                for (auto &var : const_decl->vars) {
                    if (var->value != nullptr) {
                        CODEGEN_ERROR(
                            var->var->path->name,
                            "const decorations in interfaces can not have a default value.");
                        break;
                    }
                }
            }
            default: {
                CODEGEN_ERROR(self,
                              "'" + decl->getNodeName() +
                                  "' is not allowed in an interface, remove it.");
                return;
            }
        }
    }

    ADD_TOKEN(CXX_TEMPLATE);
    ADD_TOKEN(CXX_LESS);

    if (!node.generics->params->params.empty()) {
        for (auto &param : node.generics->params->params) {
            if (param->var->path->name.value() == "self") {
                ADD_TOKEN(CXX_TYPENAME);
                ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "self", param->var->path->name);
            } else {
                ADD_PARAM(param);
            }

            ADD_TOKEN(CXX_COMMA);
        }

        tokens.pop_back();
    }

    ADD_TOKEN(CXX_GREATER);
    ADD_TOKEN(CXX_CONCEPT);                                                // concept
    ADD_NODE_PARAM(name);                                                  // FooInterface
    ADD_TOKEN(CXX_EQUAL);                                                  // =

    if (node.generics->bounds) {
        for (auto &bound : node.generics->bounds->bounds) {
            ADD_PARAM(bound);
            ADD_TOKEN(CXX_LOGICAL_AND);
        }
    }

    ADD_TOKEN(CXX_REQUIRES);                                               // requires
    ADD_TOKEN(CXX_LPAREN);                                                 // (
    ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "self", self);          // self
    ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, self_parm_name, self);  // _$_<timestamp>

    if (!type_map.empty()) {
        ADD_TOKEN(CXX_COMMA);

        for (auto &[var, name] : type_map) {
            if (!is_self_t(var->get()->var->type)) {
                ADD_PARAM(var->get()->var->type);
            }

            ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, name, var->get()->var->path->name);
            ADD_TOKEN(CXX_COMMA);
        }

        tokens.pop_back();
    }

    ADD_TOKEN(CXX_RPAREN);
    ADD_TOKEN(CXX_LBRACE);

    /*
    interface Foo {
        fn bar() requires <T>;
    }

    { t.template foo<int>() } -> std::same_as<int>;

    */

    //    ADD_TOKEN(CXX_LBRACE);

    /// i need to figure out all the functions that are in the interface and then all the types of
    /// all the functions
    /*
    interface Foo requires U {
        fn foo(self, a: U) -> i32;
        fn bar(self, a: i32) -> i32;
        static fn baz(a: i32) -> i32;
        fn qux(self, _: *void) -> i32;

        type FLA requires U;

        let a: float;
        const b: f64;
    }

    codegen into --->

    template <typename self, typename U>
    concept Foo = requires(self t, U $U, i32 $i32, i32 $i32, *void $void) {
        { t.foo($U) } -> std::same_as<i32>;
        { t.bar($i32) } -> std::same_as<i32>;
        { self::baz($i32) } -> std::same_as<i32>;
        { t.qux($void) } -> std::same_as<i32>;

        { t.a } -> std::same_as<f64>;
        { t.b } -> std::same_as<f64>;

        typename self::FLA;
    };
    */

    /// this map contains the refrencs of args to their normalized names in the interface
    /// so for exmaple take the following interface:
    /// interface Foo requires U {
    ///     fn foo(self, a: U) -> i32;
    ///     fn bar(self, a: i32) -> i32;
    ///     static fn baz(a: i32) -> i32;
    /// }
    /// the map would look like:
    /// {
    ///     <mem-addr>(a: U) : _$_<timestamp>_,
    ///     <mem-addr>(a: i32) : _$_<timestamp>_,
    ///     <mem-addr>(a: i32) : _$_<timestamp>_
    /// }
    /// this is used to replace the args in the function with the normalized names so that the
    /// codegen would look like this:
    /// template <typename self, typename U>
    /// concept Foo = requires(self t, U _$_<timestamp>_, i32 _$_<timestamp>_, i32 _$_<timestamp>_)
    /// {
    ///     { t.foo(_$_<timestamp>_) } -> std::same_as<i32>;
    ///     { t.bar(_$_<timestamp>_) } -> std::same_as<i32>;
    ///     { self::baz(_$_<timestamp>_) } -> std::same_as<i32>;
    /// };

    /// at this stage validation is done, now we need to only focus on the codegen
    for (auto &decl : node.body->body->body) {
        switch (decl->getNodeType()) {
            case __AST_NODE::nodes::FuncDecl: {  // fn foo(self, a: U) -> i32;
                /// if static codegen: { self::foo(a) } -> std::same_as<i32>;
                /// if instance codegen: { self_parm_name.foo(a) } -> std::same_as<i32>;
                __AST_N::NodeT<__AST_NODE::FuncDecl> func_decl =
                    __AST_N::as<__AST_NODE::FuncDecl>(decl);

                auto [$self, $static] = contains_self_static(func_decl);

                ADD_TOKEN(CXX_LBRACE);  // {

                if ($self) {  // self_parm_name.
                    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, self_parm_name);
                    ADD_TOKEN(CXX_DOT);
                } else {  // self::
                    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "self");
                    ADD_TOKEN(CXX_SCOPE_RESOLUTION);
                }

                ADD_PARAM(func_decl->name);  // foo
                ADD_TOKEN(CXX_LPAREN);       // (

                if (!func_decl->params.empty()) {
                    bool first = $self;

                    for (auto &param : func_decl->params) {
                        if (first) {
                            first = !first;
                            continue;
                        }

                        ADD_TOKEN_AS_VALUE_AT_LOC(
                            CXX_CORE_IDENTIFIER, type_map[&param], param->var->path->name);
                        ADD_TOKEN(CXX_COMMA);
                    }

                    // if there is self and the size of the params is 1, then we skip removing the
                    // comma
                    if (!$self || func_decl->params.size() != 1) {
                        tokens.pop_back();
                    }
                }

                ADD_TOKEN(CXX_RPAREN);  // )

                ADD_TOKEN(CXX_RBRACE);   // }
                ADD_TOKEN(CXX_PTR_ACC);  // ->
                ADD_TOKEN_AS_VALUE_AT_LOC(
                    CXX_CORE_IDENTIFIER, "std", func_decl->get_name_t().back());
                ADD_TOKEN(CXX_SCOPE_RESOLUTION);
                ADD_TOKEN_AS_VALUE_AT_LOC(
                    CXX_CORE_IDENTIFIER, "same_as", func_decl->get_name_t().back());

                ADD_TOKEN(CXX_LESS_THAN);

                if (func_decl->returns) {
                    if (!is_self_t(func_decl->returns)) {
                        ADD_PARAM(func_decl->returns);
                    }
                } else {
                    ADD_TOKEN(CXX_VOID);
                }

                ADD_TOKEN(CXX_GREATER_THAN);
                ADD_TOKEN(CXX_SEMICOLON);

                break;
            }

            case __AST_NODE::nodes::OpDecl: {
                /// op + fn add(self, other: self) -> self;
                /// if static codegen:
                ///    { self::add(a, b) } -> std::same_as<self>;
                ///    { a + b } -> std::same_as<self>;
                /// if instance codegen:
                ///    { self_parm_name.add(b) } -> std::same_as<self>;
                ///    { self_parm_name + b } -> std::same_as<self>;

                /// heres is all the decls:
                /// unary (prefix):  op + fn add(self) -> self;
                /// unary (postfix): op r+ fn add(self) -> self;
                /// binary:          op + fn add(self, other: self) -> self;
                /// array:           op [] fn add(self, other: self) -> self;

                __AST_N::NodeT<__AST_NODE::OpDecl> op_decl = __AST_N::as<__AST_NODE::OpDecl>(decl);

                auto [$self, $static] = contains_self_static(op_decl);

                if (op_decl->func->name != nullptr) {  // first declaration
                    ADD_TOKEN(CXX_LBRACE);

                    if ($self) {
                        ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, self_parm_name);
                        ADD_TOKEN(CXX_DOT);
                    } else {
                        ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "self");
                        ADD_TOKEN(CXX_SCOPE_RESOLUTION);
                    }

                    ADD_PARAM(op_decl->func->name);
                    ADD_TOKEN(CXX_LPAREN);

                    if (!op_decl->func->params.empty()) {
                        bool first = $self;

                        for (auto &param : op_decl->func->params) {
                            if (first) {
                                first = !first;
                                continue;
                            }

                            ADD_TOKEN_AS_VALUE_AT_LOC(
                                CXX_CORE_IDENTIFIER, type_map[&param], param->var->path->name);
                            ADD_TOKEN(CXX_COMMA);
                        }

                        if (!$self || op_decl->func->params.size() != 1) {
                            tokens.pop_back();
                        }
                    }

                    ADD_TOKEN(CXX_RPAREN);

                    ADD_TOKEN(CXX_RBRACE);
                    ADD_TOKEN(CXX_PTR_ACC);
                    ADD_TOKEN_AS_VALUE_AT_LOC(
                        CXX_CORE_IDENTIFIER, "std", op_decl->func->get_name_t().back());
                    ADD_TOKEN(CXX_SCOPE_RESOLUTION);
                    ADD_TOKEN_AS_VALUE_AT_LOC(
                        CXX_CORE_IDENTIFIER, "same_as", op_decl->func->get_name_t().back());
                    ADD_TOKEN(CXX_LESS_THAN);

                    if (op_decl->func->returns) {
                        if (!is_self_t(op_decl->func->returns)) {
                            ADD_PARAM(op_decl->func->returns);
                        }
                    } else {
                        ADD_TOKEN(CXX_VOID);
                    }

                    ADD_TOKEN(CXX_GREATER_THAN);
                    ADD_TOKEN(CXX_SEMICOLON);
                }

                {  /// second declaration
                   /// for binary: `{a + b}` | `{self_parm_name + b}`
                   /// for unary: `{+a}` | `{self_parm_name}`
                    ADD_TOKEN(CXX_LBRACE);

                    /// identify if the op is a unary, binary or a array op
                    OperatorType op_type = determine_operator_type(op_decl);

                    if (op_type == OperatorType::None) {
                        if (op_decl->op.size() >= 1) {
                            CODEGEN_ERROR(op_decl->op.front(),
                                          "Invalid operator/parameters for operator overload");
                        } else {
                            CODEGEN_ERROR(op_decl->func->get_name_t().back(),
                                          "Invalid operator/parameters for operator overload");
                        }

                        break;
                    }

                    switch (op_type) {
                        case OperatorType::UnaryPrefix: {  /// codegen { +a } | { +self_name_param }
                            for (auto &tok : op_decl->op) {
                                ADD_TOKEN_AS_VALUE_AT_LOC(CXX_OPERATOR, tok.value(), tok);
                            }

                            if ($self) {
                                ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, self_parm_name);
                            } else {
                                ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER,
                                                   type_map[&op_decl->func->params.front()]);
                            }

                            break;
                        }
                        case OperatorType::UnaryPostfix: {
                            if ($self) {
                                ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, self_parm_name);
                            } else {
                                ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER,
                                                   type_map[&op_decl->func->params.front()]);
                            }

                            for (auto &tok : op_decl->op) {
                                if (tok == __TOKEN_N::OPERATOR_R_INC ||
                                    tok == __TOKEN_N::OPERATOR_R_DEC) {
                                    ADD_TOKEN_AS_VALUE_AT_LOC(
                                        CXX_OPERATOR,
                                        tok == __TOKEN_N::OPERATOR_R_INC ? "++" : "--",
                                        tok);
                                } else {
                                    ADD_TOKEN_AS_VALUE_AT_LOC(CXX_OPERATOR, tok.value(), tok);
                                }
                            }

                            break;
                        }
                        case OperatorType::Binary: {
                            if ($self) {
                                ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, self_parm_name);
                            } else {
                                ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER,
                                                   type_map[&op_decl->func->params.front()]);
                            }

                            for (auto &tok : op_decl->op) {
                                ADD_TOKEN_AS_VALUE_AT_LOC(CXX_OPERATOR, tok.value(), tok);
                            }

                            ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER,
                                               type_map[&op_decl->func->params.back()]);

                            break;
                        }
                        case OperatorType::Array:
                            if ($self) {
                                ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, self_parm_name);
                            } else {
                                ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER,
                                                   type_map[&op_decl->func->params.front()]);
                            }

                            ADD_TOKEN(CXX_LBRACKET);
                            ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER,
                                               type_map[&op_decl->func->params.back()]);
                            ADD_TOKEN(CXX_RBRACKET);

                            break;
                        case OperatorType::Call: {
                            if ($self) {
                                ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, self_parm_name);
                            } else {
                                ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER,
                                                   type_map[&op_decl->func->params.front()]);
                            }

                            ADD_TOKEN(CXX_LPAREN);

                            bool first = $self;

                            if (!op_decl->func->params.empty()) {
                                for (auto &param : op_decl->func->params) {
                                    if (first) {
                                        first = !first;
                                        continue;
                                    }

                                    ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER,
                                                              type_map[&param],
                                                              param->var->path->name);
                                    ADD_TOKEN(CXX_COMMA);
                                }

                                if (!$self || op_decl->func->params.size() != 1) {
                                    tokens.pop_back();
                                }
                            }

                            break;
                        }
                        case OperatorType::None: {
                            CODEGEN_ERROR(op_decl->op.front(),
                                          "Invalid operator/parameters for operator overload");
                            break;
                        }
                    }

                    ADD_TOKEN(CXX_RBRACE);
                    ADD_TOKEN(CXX_PTR_ACC);
                    ADD_TOKEN_AS_VALUE_AT_LOC(
                        CXX_CORE_IDENTIFIER, "std", op_decl->func->get_name_t().back());
                    ADD_TOKEN(CXX_SCOPE_RESOLUTION);
                    ADD_TOKEN_AS_VALUE_AT_LOC(
                        CXX_CORE_IDENTIFIER, "same_as", op_decl->func->get_name_t().back());
                    ADD_TOKEN(CXX_LESS_THAN);

                    if (op_decl->func->returns) {
                        if (!is_self_t(op_decl->func->returns)) {
                            ADD_PARAM(op_decl->func->returns);
                        }
                    } else {
                        ADD_TOKEN(CXX_VOID);
                    }

                    ADD_TOKEN(CXX_GREATER_THAN);
                    ADD_TOKEN(CXX_SEMICOLON);
                }

                break;
            }

            case __AST_NODE::nodes::LetDecl: {  // let foo: i32;
                /// if static codegen: { self::foo } -> std::same_as<i32>;
                /// if instance codegen: { self_parm_name.foo } -> std::same_as<i32>;

                __AST_N::NodeT<__AST_NODE::LetDecl> let_decl =
                    __AST_N::as<__AST_NODE::LetDecl>(decl);

                for (auto &var : let_decl->vars) {
                    ADD_TOKEN(CXX_LBRACE);

                    if (let_decl->modifiers.contains(token::tokens::KEYWORD_STATIC)) {
                        ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "self");
                        ADD_TOKEN(CXX_SCOPE_RESOLUTION);
                        ADD_PARAM(var->var->path);
                    } else {
                        ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, self_parm_name);
                        ADD_TOKEN(CXX_DOT);
                        ADD_PARAM(var->var->path);
                    }

                    ADD_TOKEN(CXX_RBRACE);
                    ADD_TOKEN(CXX_PTR_ACC);
                    ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "std", var->var->path->name);
                    ADD_TOKEN(CXX_SCOPE_RESOLUTION);
                    ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "same_as", var->var->path->name);
                    ADD_TOKEN(CXX_LESS_THAN);

                    if (var->var->type) {
                        if (!is_self_t(var->var->type)) {
                            ADD_PARAM(var->var->type);
                        }

                        /// now new check, since we are in an interface, we need to make sure that
                        /// the the checked var type has to be a reference type, so we add a `&` to
                        /// the type unless it is a reference type

                        auto type = var->var->type;

                        if (var->var->type->value != nullptr &&
                            !let_decl->modifiers.contains(token::tokens::KEYWORD_STATIC)) {

                            if (type->value->getNodeType() == __AST_NODE::nodes::IdentExpr) {
                                ADD_TOKEN(CXX_AMPERSAND);
                            } else if (type->value->getNodeType() == __AST_NODE::nodes::UnaryExpr) {
                                __AST_N::NodeT<__AST_NODE::UnaryExpr> unary =
                                    __AST_N::as<__AST_NODE::UnaryExpr>(type->value);

                                if (unary->op == __TOKEN_N::OPERATOR_MUL) {
                                    ADD_TOKEN(CXX_AMPERSAND);
                                }
                            }
                        } else {
                            CODEGEN_ERROR(var->var->path->name,
                                          "bad type for variable declaration in interface");
                        }
                    } else {
                        CODEGEN_ERROR(var->var->path->name,
                                      "variable declarations in interfaces must have a type.");
                    }

                    ADD_TOKEN(CXX_GREATER_THAN);
                    ADD_TOKEN(CXX_SEMICOLON);
                }

                break;
            }

            case __AST_NODE::nodes::ConstDecl: {
                __AST_N::NodeT<__AST_NODE::ConstDecl> const_decl =
                    __AST_N::as<__AST_NODE::ConstDecl>(decl);

                for (auto &var : const_decl->vars) {
                    ADD_TOKEN(CXX_LBRACE);

                    if (const_decl->modifiers.contains(token::tokens::KEYWORD_STATIC)) {
                        ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "self");
                        ADD_TOKEN(CXX_SCOPE_RESOLUTION);
                        ADD_PARAM(var->var->path);
                    } else {
                        ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, self_parm_name);
                        ADD_TOKEN(CXX_DOT);
                        ADD_PARAM(var->var->path);
                    }

                    ADD_TOKEN(CXX_RBRACE);
                    ADD_TOKEN(CXX_PTR_ACC);
                    ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "std", var->var->path->name);
                    ADD_TOKEN(CXX_SCOPE_RESOLUTION);
                    ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "same_as", var->var->path->name);
                    ADD_TOKEN(CXX_LESS_THAN);

                    if (var->var->type) {
                        if (!is_self_t(var->var->type)) {
                            ADD_PARAM(var->var->type);
                        }
                    }

                    ADD_TOKEN(CXX_GREATER_THAN);
                    ADD_TOKEN(CXX_SEMICOLON);
                }

                break;
            }

            default: {
                CODEGEN_ERROR(self,
                              "'" + decl->getNodeName() +
                                  "' is not allowed in an interface, remove it.");
                return;
            }
        }
    }

    ADD_TOKEN(CXX_RBRACE);
    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(EnumDecl) {

    ADD_TOKEN(CXX_ENUM);
    ADD_TOKEN(CXX_STRUCT);  // Same as enum class, but it makes more sense here to be a struct
    ADD_NODE_PARAM(name);
    ADD_TOKEN(CXX_COLON);

    // bool has_neg= false;

    // for (auto& mem : node.members) {
    //     if (mem->value) {
    //         mem->value.get().
    //     }
    // }

    if (node.derives) {           //
        ADD_NODE_PARAM(derives);  //
    } else {                      // TODO: after sign is checked use: ADD_TOKEN(CXX_UNSIGNED);
        // if (node.members.size() >=256 )      { ADD_TOKEN(CXX_CHAR);}
        // else if (node.members.size()  ) { ADD_TOKEN(CXX_SHORT);}
        // else if (node.members.size()  ) { ADD_TOKEN(CXX_CHAR);}
        // TODO: SIZING restrictions based on number of elements
    }

    BRACE_DELIMIT(           //
        COMMA_SEP(members);  //
    );

    ADD_TOKEN(CXX_SEMICOLON);
}

CX_VISIT_IMPL(TypeDecl) {
    ADD_NODE_PARAM(generics);
    ADD_TOKEN(CXX_USING);
    ADD_NODE_PARAM(name);
    ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, "=");
    ADD_NODE_PARAM(type);
}

CX_VISIT_IMPL_VA(FuncDecl, bool no_return_t) {
    ADD_NODE_PARAM(generics);

    // add the modifiers
    // 'inline' | 'async' | 'static' | 'const' | 'eval'
    // codegen:
    //      inline -> inline
    //      async -> different codegen (not supported yet)
    //      static -> static
    //      const -> special case codegen
    //      eval -> constexpr
    //      const eval -> consteval

    if (node.modifiers.contains(token::tokens::KEYWORD_INLINE)) {
        ADD_TOKEN_AT_LOC(CXX_INLINE, node.modifiers.get(token::tokens::KEYWORD_INLINE));
    }

    if (node.modifiers.contains(token::tokens::KEYWORD_STATIC)) {
        ADD_TOKEN_AT_LOC(CXX_STATIC, node.modifiers.get(token::tokens::KEYWORD_STATIC));
    }

    if (node.modifiers.contains(token::tokens::KEYWORD_CONST) &&
        node.modifiers.contains(token::tokens::KEYWORD_EVAL)) {
        ADD_TOKEN_AT_LOC(CXX_CONSTEVAL, node.modifiers.get(token::tokens::KEYWORD_EVAL));
    } else if (node.modifiers.contains(token::tokens::KEYWORD_EVAL)) {
        ADD_TOKEN_AT_LOC(CXX_CONSTEXPR, node.modifiers.get(token::tokens::KEYWORD_EVAL));
    }

    if (node.name == nullptr) {
        throw std::runtime_error(
            std::string(colors::fg16::green) + std::string(__FILE__) + ":" +
            std::to_string(__LINE__) + colors::reset + std::string(" - ") +
            "Function Declaration is missing the name param (ub), open an issue on github.");
    }

    if (!no_return_t) {
        ADD_TOKEN(CXX_AUTO);  // auto
    }

    ADD_NODE_PARAM(name);   // name
    PAREN_DELIMIT(          //
        COMMA_SEP(params);  // (params)
    );

    if (node.modifiers.contains(token::tokens::KEYWORD_CONST)) {
        ADD_TOKEN_AT_LOC(CXX_CONST, node.modifiers.get(token::tokens::KEYWORD_CONST));
    }

    if (!no_return_t) {
        ADD_TOKEN(CXX_PTR_ACC);  // ->

        if (node.returns != nullptr) {  // return type
            ADD_NODE_PARAM(returns);
        } else {
            ADD_TOKEN(CXX_VOID);
        }
    }

    NO_EMIT_FORWARD_DECL_SEMICOLON;

    if (node.body) {
        ADD_NODE_PARAM(body);  // TODO: should only error in interfaces
    };
}

CX_VISIT_IMPL(VarDecl) {
    // if (node.var->type, ADD_PARAM(node.var->type);) else { ADD_TOKEN(CXX_AUTO); }

    ADD_NODE_PARAM(var);

    if (node.value) {
        ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, "=");
        ADD_NODE_PARAM(value);
    }
}

CX_VISIT_IMPL(FFIDecl) {
    using namespace __AST_N;
    if (node.name->value.value() != "\"c++\"") {
        throw std::runtime_error("Only C++ is supported at the moment");
    }

    if (node.value->getNodeType() == node::nodes::ImportState) {
        NodeT<node::ImportState> import = __AST_N::as<node::ImportState>(node.value);

        if (import->type != node::ImportState::Type::Single) {
            throw std::runtime_error("Only single imports are supported at the moment");
        }

        NodeT<node::SingleImport> single = __AST_N::as<node::SingleImport>(import->import);

        if (single->type == node::SingleImport::Type::Module) {
            NodeT<node::ScopePathExpr> path = __AST_N::as<node::ScopePathExpr>(single->path);

            ADD_TOKEN(CXX_IMPORT);

            for (auto &part : path->path) {
                ADD_PARAM(part);
                ADD_TOKEN(CXX_DOT);
            }

            if (path->path.size() > 0) {
                ADD_PARAM(path->access);
            }

            ADD_TOKEN(CXX_SEMICOLON);
        } else {
            ADD_TOKEN(CXX_PP_INCLUDE);
            ADD_TOKEN_AS_TOKEN(CXX_CORE_LITERAL,
                               __AST_N::as<node::LiteralExpr>(single->path)->value);
        }

    } else {
        throw std::runtime_error("Only string literals are supported at the moment");
    }
}

CX_VISIT_IMPL(OpDecl) {

    // Add the function normally if it has a name
    if (node.func->name != nullptr) {
        ADD_NODE_PARAM(func);
    }

    if (node.func->generics) {  //
        ADD_NODE_PARAM(func->generics);
    };

    ADD_TOKEN(CXX_INLINE);     // inline the operator
    if (node.func->returns) {  //
        ADD_NODE_PARAM(func->returns);
    } else {
        ADD_TOKEN(CXX_VOID);
    }

    // if (node.func->name == nullptr) {
    //     print("error");
    //     throw std::runtime_error("This is bad");
    // }

    ADD_TOKEN(CXX_OPERATOR);

    for (auto &tok : node.op) {
        ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, tok.value());
    }

    PAREN_DELIMIT(                //
        COMMA_SEP(func->params);  //
    );

    if (node.func->body) {
        // ADD_NODE_PARAM(func->body);  // TODO: should only error in interfaces
        BRACE_DELIMIT(
            ADD_TOKEN(CXX_RETURN);       //
            ADD_NODE_PARAM(func->name);  //
            PAREN_DELIMIT(               //

                for (auto &param
                     : node.func->params) {
                    ADD_PARAM(param->var->path);
                    ADD_TOKEN(CXX_COMMA);
                }

                if (node.func->params.size() > 0) {
                    this->tokens
                        .pop_back();  // TODO: make better: remove last `,` , make better in the
                });
            ADD_TOKEN(CXX_SEMICOLON););
    };
}

void generator::CXIR::CXIR::visit(__AST_NODE::Program &node) {
    std::erase_if(node.children, [&](const auto &child) {
        if (child->getNodeType() == __AST_NODE::nodes::FFIDecl) {
            child->accept(*this);
            return true;
        }
        return false;
    });

    std::string _namespace = sanitize_string(node.get_file_name());

    error::NAMESPACE_MAP[_namespace] =
        sanitize_string(std::filesystem::path(node.get_file_name()).stem().generic_string());

    // insert header guards
    ADD_TOKEN(CXX_PP_IFNDEF);
    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, _namespace + "_M");
    ADD_TOKEN(CXX_PP_DEFINE);
    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, _namespace + "_M");

    ADD_TOKEN(CXX_NAMESPACE);

    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "helix");
    ADD_TOKEN(CXX_SCOPE_RESOLUTION);
    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, _namespace);

    ADD_TOKEN(CXX_LBRACE);

    std::vector<__AST_N::NodeT<__AST_NODE::FuncDecl>> main_funcs;

    std::for_each(node.children.begin(), node.children.end(), [&](const auto &child) {
        if (child->getNodeType() == __AST_NODE::nodes::FuncDecl) {
            __AST_N::NodeT<__AST_NODE::FuncDecl> func = __AST_N::as<__AST_NODE::FuncDecl>(child);
            auto                                 name = func->get_name_t();

            if (!name.empty() && name.size() == 1) {
                /// all allowed main functions in all platforms of c++ are:
                /// main, wmain, WinMain, wWinMain, _tmain, _tWinMain

                if (name[0].value() == "main" || name[0].value() == "_main" ||
                    name[0].value() == "wmain" || name[0].value() == "WinMain" ||
                    name[0].value() == "wWinMain" || name[0].value() == "_tmain" ||
                    name[0].value() == "_tWinMain" ||
                    (!node.entry.empty() && name[0].value() == node.entry)) {
                    if (node.entry.empty() || name[0].value() != node.entry) {
                        /// there must be a return type
                        if (func->returns == nullptr) {
                            CODEGEN_ERROR(name[0], "main functions must have a return type (i32)");
                        }
                    }

                    auto &func_body = func->body->body->body;

                    /// insert an __inline_cpp("using namespace helix::_namespace;") at the start of
                    /// the main function
                    auto make_token = [&node](token::tokens kind, const std::string &value) {
                        return token::Token(kind, node.get_file_name(), value);
                    };

                    token::TokenList inline_cpp = {
                        make_token(token::tokens::IDENTIFIER, "__inline_cpp"),
                        make_token(token::tokens::PUNCTUATION_OPEN_PAREN, "("),
                        make_token(token::tokens::LITERAL_STRING,
                                   "\"using namespace helix:: " + _namespace + "\""),
                        make_token(token::tokens::PUNCTUATION_CLOSE_PAREN, ")"),
                        make_token(token::tokens::PUNCTUATION_SEMICOLON, ";"),
                        make_token(token::tokens::EOF_TOKEN, "")};

                    token::TokenList::TokenListIter inline_cpp_iter = inline_cpp.begin();

                    __AST_NODE::Statement                       parser(inline_cpp_iter);
                    __AST_N::ParseResult<__AST_NODE::ExprState> inline_cpp_node =
                        parser.parse<__AST_NODE::ExprState>();

                    func_body.insert(func_body.begin(), inline_cpp_node.value());
                    main_funcs.push_back(func);

                    return;
                }
            }
        }

        child->accept(*this);
    });

    tokens.push_back(std::make_unique<CX_Token>(cxir_tokens::CXX_RBRACE));

    for (auto &func : main_funcs) {
        func->accept(*this);
    }

    tokens.push_back(std::make_unique<CX_Token>(cxir_tokens::CXX_CORE_IDENTIFIER, "#endif"));
}
