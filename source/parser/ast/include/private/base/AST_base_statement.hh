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

#ifndef __AST_BASE_STATEMENT_H__
#define __AST_BASE_STATEMENT_H__

#include "parser/ast/include/config/AST_config.def"
#include "parser/ast/include/private/AST_generate.hh"
#include "parser/ast/include/private/base/AST_base_expression.hh"
#include "parser/ast/include/types/AST_types.hh"

__AST_NODE_BEGIN {
    /*
     *  Statement class
     *
     *  This class is responsible for parsing the statement grammar.
     *  It is a recursive descent parser that uses the token list
     *  to parse the statement grammar.
     *
     *  (its very bad quality but will be improved when written in helix)
     *
     *  usage:
     *     Statement state(tokens);
     *     NodeT<> node = state.parse();
     *
     *     // or
     *
     *     NodeT<...> node = state.parse<...>();
     */
    class Statement {  // THIS IS NOT A NODE
        template <typename T = Node>
        using p_r = parser ::ast ::ParseResult<T>;
        token ::TokenList ::TokenListIter &iter;
        std::shared_ptr<parser::preprocessor::ImportProcessor> import_processor;

      public:
        Statement()                             = delete;
        Statement(const Statement &)            = default;
        Statement &operator=(const Statement &) = delete;
        Statement(Statement &&)                 = default;
        Statement &operator=(Statement &&)      = delete;
        ~Statement()                            = default;
        p_r<> parse(std::shared_ptr<__TOKEN_N::TokenList> = nullptr);
        explicit Statement(token ::TokenList ::TokenListIter &iter, std::shared_ptr<parser::preprocessor::ImportProcessor> import_processor = nullptr)
            : iter(iter)
            , import_processor(import_processor)
            , expr_parser(iter) {};

        template <typename T, typename... Args>
        ParseResult<T> parse(Args &&...args) { /* NOLINT */
            if constexpr (std ::same_as<T, NamedVarSpecifier>) {
                return parse_NamedVarSpecifier(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, NamedVarSpecifierList>) {
                return parse_NamedVarSpecifierList(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, ForPyStatementCore>) {
                return parse_ForPyStatementCore(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, ForCStatementCore>) {
                return parse_ForCStatementCore(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, ForState>) {
                return parse_ForState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, WhileState>) {
                return parse_WhileState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, ElseState>) {
                return parse_ElseState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, IfState>) {
                return parse_IfState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, SwitchCaseState>) {
                return parse_SwitchCaseState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, SwitchState>) {
                return parse_SwitchState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, YieldState>) {
                return parse_YieldState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, DeleteState>) {
                return parse_DeleteState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, ImportState>) {
                return parse_ImportState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, ImportItems>) {
                return parse_ImportItems(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, SingleImport>) {
                return parse_SingleImport(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, SpecImport>) {
                return parse_SpecImport(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, MultiImportState>) {
                return parse_MultiImportStatee(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, ReturnState>) {
                return parse_ReturnState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, BreakState>) {
                return parse_BreakState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, BlockState>) {
                return parse_BlockState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, SuiteState>) {
                return parse_SuiteState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, ContinueState>) {
                return parse_ContinueState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, CatchState>) {
                return parse_CatchState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, FinallyState>) {
                return parse_FinallyState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, TryState>) {
                return parse_TryState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, PanicState>) {
                return parse_PanicState(std ::forward<Args>(args)...);
            } else if constexpr (std ::same_as<T, ExprState>) {
                return parse_ExprState(std ::forward<Args>(args)...);
            };
        };

      private:
        std::vector<__TOKEN_N::Token> modifiers;
        Expression                    expr_parser;

        ParseResult<NamedVarSpecifier>     parse_NamedVarSpecifier(bool force_type = false);
        ParseResult<NamedVarSpecifierList> parse_NamedVarSpecifierList(bool force_types = false);
        ParseResult<ForPyStatementCore>    parse_ForPyStatementCore(bool skip_start = false);
        ParseResult<ForCStatementCore>     parse_ForCStatementCore(bool skip_start = false);
        ParseResult<ForState>              parse_ForState();
        ParseResult<WhileState>            parse_WhileState();
        ParseResult<IfState>               parse_IfState(bool has_const = false, bool has_eval = false);
        ParseResult<ElseState>             parse_ElseState();
        ParseResult<SwitchState>           parse_SwitchState();
        ParseResult<SwitchCaseState>       parse_SwitchCaseState();

        ParseResult<ImportState>      parse_ImportState(bool ffi_import = false);
        ParseResult<ImportItems>       parse_ImportItems();
        ParseResult<SingleImport>     parse_SingleImport();
        ParseResult<SpecImport>       parse_SpecImport(ParseResult<SingleImport> path = nullptr);
        ParseResult<MultiImportState> parse_MultiImportState();

        ParseResult<YieldState>    parse_YieldState();
        ParseResult<DeleteState>   parse_DeleteState();
        ParseResult<ReturnState>   parse_ReturnState();
        ParseResult<BreakState>    parse_BreakState();
        ParseResult<ContinueState> parse_ContinueState();
        ParseResult<ExprState>     parse_ExprState();
        ParseResult<BlockState>    parse_BlockState();
        ParseResult<TryState>      parse_TryState();
        ParseResult<CatchState>    parse_CatchState();
        ParseResult<FinallyState>  parse_FinallyState();
        ParseResult<PanicState>    parse_PanicState();
        ParseResult<SuiteState>    parse_SuiteState();
    };
}  //  namespace __AST_NODE_BEGIN

#endif  // __AST_BASE_STATEMENT_H__