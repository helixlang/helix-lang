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
//                                                                                                //
//  This file defines the AST Expression Parser, which is used to parse expressions in the AST.   //
//                                                                                                //
//===-----------------------------------------------------------------------------------------====//

#include "parser/ast/include/AST.hh"

#include "parser/ast/include/case_types.def"
#include "token/include/generate.hh"
#include "token/include/token_list.hh"

/* IDEINTIFIED

break into 2 parts the compound expressions and the single expressions

single expressions are:
    [x] Literals
    [x] Identifier
    [x] UnaryOp
    [x] FunctionCall
    [x] ScopeAccess
    [x] Parenthesized ( '(' CompoundExpression ')' )

compound expressions are:
    [-] PathAccess
    [-] DotAccess

    [-] BinaryOperation
    [-] ArrayAccess
    [-] Cast

*/

namespace parser::ast {
NodeT<Expression> get_simple_Expression(token::TokenList &tokens) {
    // Parse any simple expression
    for (auto &tok : tokens) {
        switch (tok->token_kind()) {
            case IS_LITERAL:
                return new node::Literal(tokens);

            case IS_IDENTIFIER:
                if (tok.peek().has_value() && tok.peek()->get().token_kind() == token::OPERATOR_SCOPE) {
                    return new node::ScopeAccess(tokens);
                } else if (tok.peek().has_value() && tok.peek()->get().token_kind() == token::PUNCTUATION_OPEN_PAREN) {
                    return new node::FunctionCall(tokens);
                }

                return new node::Identifier(tokens);

            case token ::PUNCTUATION_OPEN_ANGLE:
            case token ::PUNCTUATION_CLOSE_ANGLE:
            case IS_OPERATOR:
                if (tok.peek().has_value() && tok.peek()->get().token_kind() == token::PUNCTUATION_OPEN_PAREN) {
                    return new node::Parenthesized(tokens);
                }

                return new node::UnaryOp(tokens);

            case token::PUNCTUATION_OPEN_PAREN:
                return new node::Parenthesized(tokens);

            default:
                break;
        }
    }

    return nullptr;
}


NodeT<Expression> get_Expression(token::TokenList &tokens) {
    /// Parse any expression
    /// Expression ::= Literal | AnySeparatedID | BinaryOperation | UnaryOperation | FunctionCall |
    /// ParenthesizedExpression | ArrayAccess | ObjectAccess | ConditionalExpression Parsing Order

    /// The order of operations is as follows: (is this correct?)
    // Operator      ::= UnaryOp   | BinaryOp
    // Identifier    ::= DotAccess | ScopeAccess | PathAccess | FunctionCall
    // Literal       ::= Literal   | BinaryOp    | UnaryOp    | Cast         | Conditional
    // Parenthesized ::= ( Expression )

    /// The order of operations is as follows:
    // UnaryOp       ::= op ... | ... op
    // BinaryOp      ::= ... op ...
    // DotAccess     ::= ... . ...
    // ScopeAccess   ::= ... :: ...
    // PathAccess    ::= ... :: ... | ... . ...
    // FunctionCall  ::= ... ( ... )
    // Literal       ::= Literal
    // Cast          ::= ... as ...
    // Conditional   ::= ... if ... else ...
    // Parenthesized ::= ( Expression )

    return get_simple_Expression(tokens);
}

Expression::Expression() = default;
Expression::Expression(token::TokenList &tokens)
    : Node(tokens)
    , tokens(&tokens) {}
}  // namespace parser::ast