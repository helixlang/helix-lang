//===------------------------------------------ C++ ------------------------------------------====//
//                                                                                                //
//  Part of the Helix Project, under the Attribution 4.0 International license (CC BY 4.0). You   //
//  are allowed to use, modify, redistribute, and create derivative works, even for commercial    //
//  purposes, provided that you give appropriate credit, and indicate if changes were made.       //
//  For more information, please visit: https://creativecommons.org/licenses/by/4.0/              //
//                                                                                                //
//  SPDX-License-Identifier: CC-BY-4.0                                                            //
//  Copyright (c) 2024 (CC BY 4.0)                                                                //
//                                                                                                //
///====--------------------------------------------------------------------------------------====///
///                                                                                              ///
///  @file AST_types.hh                                                                          ///
///  @brief Defines common types for Abstract Syntax Tree (AST) nodes used throughout the Helix  ///
///         project. This includes types for nodes, parse results, and helpers for creating AST  ///
///         nodes.                                                                               ///
///                                                                                              ///
///  This file provides types and helper functions used in the construction and management of    ///
///     AST nodes within the Helix parser. It defines `NodeT`, a template for handling AST       ///
///     nodes, `ParseResult`, for handling parsing results (either a node or an error), and      ///
///     `NodeV`, a vector of AST nodes. Additionally, a `make_node` function is provided for     ///
///     creating new AST nodes with perfect forwarding of arguments.                             ///
///                                                                                              ///
///  @code                                                                                       ///
///  NodeT<ast::node::Type> node = make_node<ast::node::Type>(token, type);                      ///
///  NodeV<> nodes = {node};                                                                     ///
///  @endcode                                                                                    ///
///                                                                                              ///
///===---------------------------------------------------------------------------------------====///

#ifndef __AST_TYPES_H__
#define __AST_TYPES_H__

#include <expected>
#include <memory>
#include <vector>

#include "parser/ast/include/config/AST_config.def"
#include "parser/ast/include/types/AST_parse_error.hh"
#include "token/include/token.hh"

__AST_NODE_BEGIN { class Node; }

__AST_BEGIN {
    /// NodeT is a unique pointer to a T (where T is a AST node)
    template <typename T = __AST_NODE::Node>
    using NodeT = std::shared_ptr<T>;

    template <typename T = __AST_NODE::Node>  // either a node or a parse error
    using ParseResult = std::expected<NodeT<T>, ParseError>;

    /// NodeV is a vector of NodeT
    template <typename T = __AST_NODE::Node>
    using NodeV = std::vector<NodeT<T>>;

    /// make_node is a helper function to create a new node with perfect forwarding
    /// @tparam T is the type of the node
    /// @param args are the arguments to pass to the constructor of T
    /// @return a unique pointer to the new node
    template <typename T, typename... Args>
    inline constexpr NodeT<T> make_node(Args && ...args) {
        // return a heap-alloc unique pointer to the new node with
        // perfect forwarding of the arguments allowing the caller
        // to identify any errors in the arguments at compile time
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
}  // namespace __AST_BEGIN

#endif  // __AST_TYPES_H__