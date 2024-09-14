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

__AST_NODE_BEGIN {
PARSE_SIG(Identifier) {
    if (tokens->empty()) [[unlikely]] {
        return 0;
    }

    if (tokens->front().token_kind() == token::IDENTIFIER) {
        value = tokens->front();

        return 1;
    }

    return 0;
}

TEST_SIG(Identifier) {
    if (tokens->empty()) [[unlikely]] {
        return false;
    }
    return false;
}

VISITOR_IMPL(Identifier);

}  // namespace __AST_NODE_BEGIN
