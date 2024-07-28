// -*- C++ -*-
//===------------------------------------------------------------------------------------------===//
//
// Part of the Helix Project, under the Attribution 4.0 International license (CC BY 4.0).
// You are allowed to use, modify, redistribute, and create derivative works, even for commercial
// purposes, provided that you give appropriate credit, and indicate if changes were made.
// For more information, please visit: https://creativecommons.org/licenses/by/4.0/
//
// SPDX-License-Identifier: CC-BY-4.0
// Copyright (c) 2024 (CC BY 4.0)
//
//===------------------------------------------------------------------------------------------===//

#ifndef __PARSER_HH__
#define __PARSER_HH__

#include <iostream>
#include <memory>

#include "parser/ast/include/ast.hh"
#include "token/include/token.hh"
#include "token/include/token_list.hh"

namespace parser {

class ASTParser {
    explicit ASTParser(token::TokenList tokens);
};
}  // namespace parser

#endif  // __PARSER_HH__