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

CX_VISIT_IMPL(Type) {  // TODO Modifiers
    if (node.specifiers.contains(token::tokens::KEYWORD_YIELD)) {
        auto marker = node.specifiers.get(token::tokens::KEYWORD_YIELD);

        ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "helix", marker);
        ADD_TOKEN(CXX_SCOPE_RESOLUTION);

        ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "std", marker);
        ADD_TOKEN(CXX_SCOPE_RESOLUTION);

        ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "$generator", marker);
        ANGLE_DELIMIT(ADD_NODE_PARAM(value); ADD_NODE_PARAM(generics););

        return;
    }

    if (node.nullable) {
        __TOKEN_N::Token marker;

        if (node.value->getNodeType() == __AST_NODE::nodes::UnaryExpr) {
            marker = __AST_N::as<__AST_NODE::UnaryExpr>(node.value)->op;
        }

        ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "helix", marker);
        ADD_TOKEN(CXX_SCOPE_RESOLUTION);

        ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "std", marker);
        ADD_TOKEN(CXX_SCOPE_RESOLUTION);

        ADD_TOKEN_AS_VALUE_AT_LOC(CXX_CORE_IDENTIFIER, "$question", marker);
        ANGLE_DELIMIT(ADD_NODE_PARAM(value); ADD_NODE_PARAM(generics););

        return;
    }

    if (node.is_fn_ptr) {
        CXIR_NOT_IMPLEMENTED;
    }

    ADD_NODE_PARAM(value);
    ADD_NODE_PARAM(generics);
}
