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

#include <concepts>
#include <vector>
#include "generator/include/config/Gen_config.def"
#include "parser/ast/include/config/AST_config.def"
#include "parser/ast/include/nodes/AST_declarations.hh"
#include "parser/ast/include/types/AST_types.hh"
#include "utils.hh"


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

    add_func_modifiers(this, node.modifiers);

    if (!no_return_t) {
        ADD_TOKEN(CXX_AUTO);  // auto
    }

    ADD_NODE_PARAM(name);   // name
    PAREN_DELIMIT(          //
        COMMA_SEP(params);  // (params)
    );

    add_func_specifiers(this, node.modifiers);

    if (!no_return_t) {
        ADD_TOKEN(CXX_PTR_ACC);  // ->

        if (node.returns != nullptr) {  // return type
            ADD_NODE_PARAM(returns);
        } else {
            ADD_TOKEN_AT_LOC(CXX_VOID, node.marker);
        }
    }

    NO_EMIT_FORWARD_DECL_SEMICOLON;

    if (node.body && node.body->body) {
        // adds and removes any nested functions
        BRACE_DELIMIT( //
            std::erase_if(node.body->body->body, ModifyNestedFunctions(this));
        ); //
    } else {
        ADD_TOKEN(CXX_SEMICOLON);
    }
}
