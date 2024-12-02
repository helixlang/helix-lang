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

CX_VISIT_IMPL(StructDecl) {
    // TODO: only enums, strcuts, types, unnamed ops, and vars (let or const)
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

    if (node.body != nullptr) {
        for (auto &decl : node.body->body->body) {
            switch (decl->getNodeType()) {
                case parser::ast::node::nodes::EnumDecl:
                case parser::ast::node::nodes::StructDecl:
                case parser::ast::node::nodes::TypeDecl:
                case parser::ast::node::nodes::OpDecl:
                case parser::ast::node::nodes::LetDecl:
                case parser::ast::node::nodes::ConstDecl:
                    break;
                default:
                    CODEGEN_ERROR(node.name->name,
                                  "strcut declaration cannot have a node of type: '" +
                                      decl->getNodeName() +
                                      "', strcut can only contain: enums, types, strcuts, unnamed "
                                      "ops, and let/const declarations.")
                    continue;
            }

            if (decl->getNodeType() == parser::ast::node::nodes::OpDecl) {
                parser::ast::NodeT<parser::ast::node::OpDecl> op_decl =
                    __AST_N::as<parser::ast::node::OpDecl>(decl);
                if (op_decl->func->name) {
                    auto maerker = op_decl->func->name->get_back_name();
                    CODEGEN_ERROR(maerker,
                                  "strcut declaration can not have named operators, remove the "
                                  "named alias here.");
                    continue;
                }
            }

            decl->accept(*this);
        }
    }

    ADD_TOKEN(CXX_SEMICOLON);
}
