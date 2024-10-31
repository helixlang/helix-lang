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

#ifndef __AST_GENERATE_H__
#define __AST_GENERATE_H__

#include "parser/ast/include/config/AST_Annotations.def"
#include "parser/ast/include/config/AST_Declarations.def"
#include "parser/ast/include/config/AST_Expressions.def"
#include "parser/ast/include/config/AST_Statements.def"
#include "parser/ast/include/config/AST_config.def"

__AST_NODE_BEGIN {
    class Node;
    class Expression;
    class Statement;
    class Declaration;
    class Annotation;

    GENERATE_NODES_ENUM;
    GENERATE_NODES_FORWARD_DECLS;
}

#endif  // __AST_GENERATE_H__