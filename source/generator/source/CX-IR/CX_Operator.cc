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

#include "utils.hh"

CX_VISIT_IMPL_VA(OpDecl, bool in_udt) {
    OpType       op_t  = OpType(node, in_udt);
    auto         _node = std::make_shared<__AST_NODE::OpDecl>(node);
    token::Token tok;

    /// FIXME: really have to add markers to the rewrite of the compiler
    if (in_udt) {
        if (op_t.type == OpType::Error) {
            return;
        }

        tok = *op_t.tok;
    } else {
        if (!node.op.empty()) {
            tok = node.op.front();
        } else {
            auto name = node.func->get_name_t();
            if (!name.empty()) {
                tok = name.back();
            } else {
                tok = node.func->marker;
            }
        }
    }

    handle_static_self_fn_decl(_node, tok, in_udt);

    // ---------------------------- add generator state ---------------------------- //
    if (in_udt && op_t.type == OpType::GeneratorOp) {
        // `private: mutable helix::generator<return_type>* $gen_state = nullptr; public:`
        ADD_TOKEN(CXX_PRIVATE);
        ADD_TOKEN(CXX_COLON);

        ADD_TOKEN(CXX_MUTABLE);
        ADD_NODE_PARAM(func->returns);
        ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "$gen_state", tok);

        ADD_TOKEN(CXX_ASSIGN);
        ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "$generator", tok);
        PAREN_DELIMIT();

        ADD_TOKEN(CXX_SEMICOLON);

        ADD_TOKEN(CXX_PUBLIC);
        ADD_TOKEN(CXX_COLON);
    }

    // ---------------------------- operator declaration ---------------------------- //

    if (node.func->generics) {  //
        ADD_NODE_PARAM(func->generics);
    };

    if (!node.modifiers.contains(__TOKEN_N::KEYWORD_INLINE)) {
        ADD_TOKEN(CXX_INLINE);  // inline the operator
    }

    add_func_modifiers(this, node.modifiers);

    ADD_TOKEN(CXX_AUTO);

    if (in_udt && op_t.type != OpType::None) {
        // if its a generator op
        // the codegen makes 3 functions:
        // 1. the generator function: auto $generator() -> ... {}
        // 2. the begin function: auto begin() {return $generator().begin(); }
        // 3. the end function: auto end() {return $generator().end(); }

        if (op_t.type == OpType::GeneratorOp) {
            /// add the fucntion
            ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "$generator", *op_t.tok);
        }

        // if its a contains op
        // the codegen makes 1 function: auto $contains() -> bool {}

        else if (op_t.type == OpType::ContainsOp) {
            /// add the fucntion
            ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "$contains", *op_t.tok);
        }

        // if its a cast op
        // the codegen makes 1 function: auto $cast(<type>*) -> <type> {}

        else if (op_t.type == OpType::CastOp) {
            /// add the fucntion
            ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "$cast", *op_t.tok);

            auto type =
                __AST_N::make_node<__AST_NODE::Type>(__AST_N::make_node<__AST_NODE::UnaryExpr>(
                    node.func->returns,
                    __TOKEN_N::Token(__TOKEN_N::OPERATOR_MUL, "*", *op_t.tok),
                    __AST_NODE::UnaryExpr::PosType::PreFix,
                    true));

            auto param = __AST_N::make_node<__AST_NODE::VarDecl>(
                __AST_N::make_node<__AST_NODE::NamedVarSpecifier>(
                    __AST_N::make_node<__AST_NODE::IdentExpr>(
                        __TOKEN_N::Token()  // whitespace token
                        ),
                    type));

            node.func->params.emplace_back(param);
        }
    } else {
        ADD_TOKEN(CXX_OPERATOR);

        for (auto &tok : node.op) {
            ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, tok.value());
        }
    }

    ADD_TOKEN(CXX_LPAREN);
    if (!node.func->params.empty()) {
        for (auto &param : node.func->params) {
            ADD_PARAM(param);
            ADD_TOKEN(CXX_COMMA);
        }

        this->tokens.pop_back();
    }
    ADD_TOKEN(CXX_RPAREN);

    add_func_specifiers(this, node.modifiers);

    ADD_TOKEN(CXX_PTR_ACC);

    if (node.func->returns) {  //
        ADD_NODE_PARAM(func->returns);
    } else {
        ADD_TOKEN_AT_LOC(CXX_VOID, node.func->marker);
    }

    if (node.func->body && node.func->body->body) {
        // adds and removes any nested functions
        BRACE_DELIMIT( //
            std::erase_if(node.func->body->body->body, ModifyNestedFunctions(this));
        ); //
    } else {
        ADD_TOKEN(CXX_SEMICOLON);
    }

    // ---------------------------- function declaration ---------------------------- //

    // add the alias function first if it has a name
    if (node.func->name != nullptr) {
        if (node.func->generics) {  //
            ADD_NODE_PARAM(func->generics);
        };

        add_func_modifiers(this, node.modifiers);

        ADD_TOKEN(CXX_AUTO);

        ADD_NODE_PARAM(func->name);

        PAREN_DELIMIT(                //
            COMMA_SEP(func->params);  //
        );

        add_func_specifiers(this, node.modifiers);

        ADD_TOKEN(CXX_PTR_ACC);
        if (node.func->returns) {  //
            ADD_NODE_PARAM(func->returns);
        } else {
            ADD_TOKEN_AT_LOC(CXX_VOID, node.func->marker);
        }

        BRACE_DELIMIT(
            ADD_TOKEN(CXX_RETURN);  //

            if (in_udt && op_t.type != OpType::None) {
                if (op_t.type == OpType::GeneratorOp) {
                    /// add the fucntion
                    ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "$generator", *op_t.tok);
                } else if (op_t.type == OpType::ContainsOp) {
                    /// add the fucntion
                    ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "$contains", *op_t.tok);
                } else if (op_t.type == OpType::CastOp) {
                    /// add the fucntion
                    ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "$cast", *op_t.tok);
                }
            } else {
                ADD_TOKEN(CXX_OPERATOR);     //
                for (auto &tok : node.op) {  //
                    ADD_TOKEN_AS_VALUE(CXX_CORE_OPERATOR, tok.value());
                }
            }

            ADD_TOKEN(CXX_LPAREN);
            if (!node.func->params.empty()) {
                for (auto &param : node.func->params) {
                    ADD_PARAM(param->var->path);
                    ADD_TOKEN(CXX_COMMA);
                }

                this->tokens.pop_back();
            } ADD_TOKEN(CXX_RPAREN);

            ADD_TOKEN(CXX_SEMICOLON);  //
        );
    }

    // ---------------------------- add generator functions ---------------------------- //

    if (in_udt && op_t.type == OpType::GeneratorOp) {
        /// add the fucntions
        // 2. the begin function:
        // auto begin() {
        //     if ($gen_state == nullptr) { $gen_state = $generator(); } return $gen_state->begin();
        // }
        // 3. the end function:
        // auto end() {
        //    if ($gen_state == nullptr) { $gen_state = $generator(); } return $gen_state->end();
        // }
        ADD_TOKEN(CXX_AUTO);
        ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "begin", tok);
        PAREN_DELIMIT();
        BRACE_DELIMIT(ADD_TOKEN(CXX_RETURN);  //
                      ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "$gen_state", *op_t.tok);
                      ADD_TOKEN(CXX_DOT);  //
                      ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "begin", *op_t.tok);
                      PAREN_DELIMIT();
                      ADD_TOKEN(CXX_SEMICOLON);  //
        );

        ADD_TOKEN(CXX_AUTO);
        ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "end", tok);
        PAREN_DELIMIT();
        BRACE_DELIMIT(ADD_TOKEN(CXX_RETURN);  //
                      ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "$gen_state", *op_t.tok);
                      ADD_TOKEN(CXX_DOT);  //
                      ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "end", *op_t.tok);
                      PAREN_DELIMIT();
                      ADD_TOKEN(CXX_SEMICOLON);  //
        );
    }
}
