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

#ifndef __AST_STATEMENTS_H__
#define __AST_STATEMENTS_H__

#include <utility>

#include "parser/ast/include/config/AST_config.def"
#include "parser/ast/include/core/AST_nodes.hh"
#include "parser/ast/include/types/AST_types.hh"

// va
// var_decl := 'let' Ident (':' E)? ('=' expr)? ';'

__AST_NODE_BEGIN {
    class NamedVarSpecifier final : public Node {
        BASE_CORE_METHODS(NamedVarSpecifier);

        // := Ident (':' E)?

        explicit NamedVarSpecifier(NodeT<IdentExpr> path, NodeT<Type> type = nullptr)
            : path(std::move(path))
            , type(std::move(type)) {
            if (type != nullptr) {
                has_type = true;
            }
        }

        NodeT<IdentExpr> path;
        NodeT<Type>      type;

        bool has_type;
    };

    class NamedVarSpecifierList final : public Node {
        BASE_CORE_METHODS(NamedVarSpecifierList);

        // := NamedVarSpecifier (',' NamedVarSpecifier)*

        explicit NamedVarSpecifierList(bool /* unused */) {}

        NodeV<NamedVarSpecifier> vars;
    };

    class ForPyStatementCore final : public Node {
        BASE_CORE_METHODS(ForPyStatementCore);

        // := NamedVarSpecifier 'in 'expr' Suite

        explicit ForPyStatementCore(bool /* unused */) {}

        token::Token                 in_marker;
        NodeT<NamedVarSpecifierList> vars;
        NodeT<>                      range;
        NodeT<>                      body;
    };

    class ForCStatementCore final : public Node {
        BASE_CORE_METHODS(ForCStatementCore);

        // := (expr)? ';' (expr)? ';' (expr)? Suite

        explicit ForCStatementCore(bool /* unused */) {}

        NodeT<>           init;
        NodeT<>           condition;
        NodeT<>           update;
        NodeT<SuiteState> body;
    };

    class ForState final : public Node {
        BASE_CORE_METHODS(ForState);

        // := 'for' (ForPyStatementCore | ForCStatementCore)

        enum class ForType {
            Python,
            C,
        };

        ForState(NodeT<> core, ForType type)
            : core(std::move(core))
            , type(type) {}

        NodeT<> core;
        ForType type;
    };

    class WhileState final : public Node {
        BASE_CORE_METHODS(WhileState);

        // := 'while' expr Suite

        explicit WhileState(NodeT<> condition, NodeT<SuiteState> body)
            : condition(std::move(condition))
            , body(std::move(body)) {}

        NodeT<>           condition;
        NodeT<SuiteState> body;
    };

    class IfState final : public Node {
        BASE_CORE_METHODS(IfState);

        // := 'if' expr Suite (ElseState)?

        enum class IfType {
            If,
            Unless,
        };

        explicit IfState(NodeT<> condition)
            : condition(std::move(condition)) {}

        NodeT<>           condition;
        NodeT<SuiteState> body;
        NodeV<ElseState>  else_body;
        IfType            type = IfType::If;
    };

    class ElseState final : public Node {
        BASE_CORE_METHODS(ElseState);

        // := 'else' Suite | 'else' ('if' | 'unless') E Suite

        enum class ElseType {
            Else,
            ElseIf,
            ElseUnless,
        };

        explicit ElseState(bool /* unused */) {}

        NodeT<>           condition;
        NodeT<SuiteState> body;
        ElseType          type = ElseType::Else;
    };

    class SwitchState final : public Node {
        BASE_CORE_METHODS(SwitchState);

        // := 'switch' expr '{' SwitchCaseState* '}'

        SwitchState(NodeT<> condition)
            : condition(std::move(condition)) {}

        NodeT<>                condition;
        NodeV<SwitchCaseState> cases;
    };

    class SwitchCaseState final : public Node {
        BASE_CORE_METHODS(SwitchCaseState);

        // := 'case' expr Suite | 'default' Suite

        enum class CaseType {
            Case,
            Default,
            Fallthrough,
        };

        SwitchCaseState(NodeT<> condition, NodeT<SuiteState> body, CaseType type, token::Token marker)
            : condition(std::move(condition))
            , body(std::move(body))
            , type(type)
            , marker(std::move(marker)) {}

        NodeT<>           condition;
        NodeT<SuiteState> body;
        CaseType          type;
        token::Token      marker;
    };

    class ImportState final : public Node {
        BASE_CORE_METHODS(ImportState);
    };

    class SingleImportState final : public Node {
        BASE_CORE_METHODS(SingleImportState);
    };

    class MultiImportState final : public Node {
        BASE_CORE_METHODS(MultiImportState);
    };

    class AliasState final : public Node {
        BASE_CORE_METHODS(AliasState);
    };

    class YieldState final : public Node {
        BASE_CORE_METHODS(YieldState);

        // := 'yield' expr ';'

        explicit YieldState(NodeT<> value)
            : value(std::move(value)) {}

        NodeT<> value;
    };

    class DeleteState final : public Node {
        BASE_CORE_METHODS(DeleteState);

        // := 'yield' expr ';'

        explicit DeleteState(NodeT<> value)
            : value(std::move(value)) {}

        NodeT<> value;
    };

    class ReturnState final : public Node {
        BASE_CORE_METHODS(ReturnState);

        // := 'return' expr ';'

        explicit ReturnState(NodeT<> value)
            : value(std::move(value)) {}

        NodeT<> value;
    };

    class BreakState final : public Node {
        BASE_CORE_METHODS(BreakState);

        // := 'break' ';'

        explicit BreakState(token::Token marker)
            : marker(std::move(marker)) {}

        token::Token marker;
    };

    class ContinueState final : public Node {
        BASE_CORE_METHODS(ContinueState);

        // := 'continue' ';'

        explicit ContinueState(token::Token marker)
            : marker(std::move(marker)) {}

        token::Token marker;
    };

    class ExprState final : public Node {
        BASE_CORE_METHODS(ExprState);

        // := expr ';'

        explicit ExprState(NodeT<> value)
            : value(std::move(value)) {}

        NodeT<> value;
    };

    class BlockState final : public Node {
        BASE_CORE_METHODS(BlockState);

        // := '{' Statement* '}'

        explicit BlockState(NodeV<> body)
            : body(std::move(body)) {}

        NodeV<> body;
    };

    class SuiteState final : public Node {
        BASE_CORE_METHODS(SuiteState);

        // := BlockState | (':' Statement)
        // the suite parser will either make a block node if theres a { or a single statement with
        // the :

        explicit SuiteState(NodeT<BlockState> body)
            : body(std::move(body)) {}

        NodeT<BlockState> body;
    };

    class CatchState final : public Node {
        BASE_CORE_METHODS(CatchState);

        // := 'catch' (NamedVarSpecifier ((',' NamedVarSpecifier)*)?)? SuiteState (CatchState |
        // FinallyState)?

        CatchState(NodeT<NamedVarSpecifier> catch_state, NodeT<SuiteState> body)
            : catch_state(std::move(catch_state))
            , body(std::move(body)) {}

        NodeT<NamedVarSpecifier> catch_state;
        NodeT<SuiteState>        body;
    };

    class FinallyState final : public Node {
        BASE_CORE_METHODS(FinallyState);

        // := 'finally' SuiteState

        explicit FinallyState(NodeT<SuiteState> body)
            : body(std::move(body)) {}

        NodeT<SuiteState> body;
    };

    class TryState final : public Node {
        BASE_CORE_METHODS(TryState);

        // := 'try' SuiteState (CatchState*) (FinallyState)?

        explicit TryState(NodeT<SuiteState> body)
            : body(std::move(body)) {}

        TryState(NodeT<SuiteState>   body,
                 NodeV<CatchState>   catch_states,
                 NodeT<FinallyState> finally_state = nullptr)
            : body(std::move(body))
            , catch_states(std::move(catch_states))
            , finally_state(std::move(finally_state)) {}

        NodeT<SuiteState>   body;
        NodeV<CatchState>   catch_states;
        NodeT<FinallyState> finally_state;

        bool no_catch;
    };

    class PanicState final : public Node {
        BASE_CORE_METHODS(PanicState);

        // := 'panic' E ';'

        explicit PanicState(NodeT<> expr)
            : expr(std::move(expr)) {}

        NodeT<> expr;
    };

}  // namespace __AST_NODE_BEGIN

#endif  // __AST_STATEMENTS_H__