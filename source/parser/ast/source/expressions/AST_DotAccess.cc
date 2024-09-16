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

#include "parser/ast/include/AST.hh"

#define __HIDE_FROM_LIBHELIX__

__AST_NODE_BEGIN {
PARSE_SIG(DotAccess) {
    if (tokens == nullptr || tokens->empty()) [[unlikely]] {
        return 0;
    }

    // DotAccess ::= paths ('.' paths)*

    i32 len = 0;
    
    paths.push_back(get_Expression(*tokens));

    return 0;
}

TEST_SIG(DotAccess) {
    if (tokens == nullptr || tokens->empty()) [[unlikely]] {
        return false;
    }
    return false;
}

VISITOR_IMPL(DotAccess);
}  // namespace __AST_NODE_BEGIN
