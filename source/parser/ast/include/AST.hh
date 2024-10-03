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

#ifndef __AST_H__
#define __AST_H__

#include "parser/ast/include/config/AST_config.def"
#include "parser/ast/include/private/AST_generate.hh"
#include "parser/ast/include/private/AST_classifier.hh"
#include "parser/ast/include/private/AST_context.hh"
#include "parser/ast/include/private/AST_matcher.hh"
#include "parser/ast/include/private/AST_nodes.hh"
#include "parser/ast/include/nodes/AST_declarations.hh"
#include "parser/ast/include/nodes/AST_expressions.hh"
#include "parser/ast/include/nodes/AST_statements.hh"
#include "parser/ast/include/types/AST_jsonify_visitor.hh"
#include "parser/ast/include/types/AST_types.hh"
#include "parser/ast/include/types/AST_visitor.hh"

#endif  // __AST_H__