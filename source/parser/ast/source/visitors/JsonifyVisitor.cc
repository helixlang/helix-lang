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

#include "neo-json/include/json.hh"
#include "parser/ast/include/config/AST_config.def"
#include "parser/ast/include/types/AST_jsonify_visitor.hh"

__AST_VISITOR_BEGIN {
    void Jsonify::visit(const parser ::ast ::node ::GenericArgumentExpr &node) {}
    void Jsonify::visit(const parser ::ast ::node ::GenericInvokeExpr &node) {}
    void Jsonify::visit(const parser ::ast ::node ::GenericInvokePathExpr &node) {}

    void Jsonify::visit(const parser ::ast ::node ::Type &node) {}    /* DEPRECATED */

}  // namespace __AST_BEGIN
