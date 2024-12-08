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

#ifndef __AST_BASE_DECLARATION_H__
#define __AST_BASE_DECLARATION_H__

#include "parser/ast/include/config/AST_config.def"
#include "parser/ast/include/private/AST_generate.hh"
#include "parser/ast/include/private/base/AST_base_expression.hh"
#include "parser/ast/include/private/base/AST_base_statement.hh"

__AST_NODE_BEGIN {
    /*
     *  Declaration class
     *
     *  This class is responsible for parsing the declaration grammar.
     *  It is a recursive descent parser that uses the token list
     *  to parse the declaration grammar.
     *
     *  (its very bad quality but will be improved when written in helix)
     *
     *  usage:
     *     Declaration decl(tokens);
     *     NodeT<> node = decl.parse();
     *
     *     // or
     *
     *     NodeT<...> node = decl.parse<...>();
     */
    class Declaration {
        AST_CLASS_BASE(Declaration, DECLS), state_parser(iter), expr_parser(iter) {};

        template <typename T, typename... Args>
        ParseResult<T> parse(Args &&...args) { /* NOLINT */
            if constexpr (std ::is_same_v<T, RequiresParamDecl>) {
                return parse_RequiresParamDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, RequiresParamList>) {
                return parse_RequiresParamList(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, EnumMemberDecl>) {
                return parse_EnumMemberDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, UDTDeriveDecl>) {
                return parse_UDTDeriveDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, TypeBoundList>) {
                return parse_TypeBoundList(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, TypeBoundDecl>) {
                return parse_TypeBoundDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, RequiresDecl>) {
                return parse_RequiresDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, StructDecl>) {
                return parse_StructDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, ConstDecl>) {
                return parse_ConstDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, ClassDecl>) {
                return parse_ClassDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, ExtendDecl>) {
                return parse_ExtendDecl(std ::forward<Args>(args)...); 
            } else if constexpr (std ::is_same_v<T, InterDecl>) {
                return parse_InterDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, EnumDecl>) {
                return parse_EnumDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, TypeDecl>) {
                return parse_TypeDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, FuncDecl>) {
                return parse_FuncDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, VarDecl>) {
                return parse_VarDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, FFIDecl>) {
                return parse_FFIDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, LetDecl>) {
                return parse_LetDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, OpDecl>) {
                return parse_OpDecl(std ::forward<Args>(args)...);
            } else if constexpr (std ::is_same_v<T, ModuleDecl>) {
                return parse_ModuleDecl(std ::forward<Args>(args)...);
            }
        }

      private:
        Statement  state_parser;
        Expression expr_parser;

        ParseResult<RequiresParamDecl> parse_RequiresParamDecl();
        ParseResult<RequiresParamList> parse_RequiresParamList();
        ParseResult<EnumMemberDecl>    parse_EnumMemberDecl();
        ParseResult<UDTDeriveDecl>     parse_UDTDeriveDecl();
        ParseResult<TypeBoundList>     parse_TypeBoundList();
        ParseResult<TypeBoundDecl>     parse_TypeBoundDecl();
        ParseResult<RequiresDecl>      parse_RequiresDecl();
        ParseResult<ModuleDecl>
        parse_ModuleDecl(const std::shared_ptr<__TOKEN_N::TokenList> &modifiers = nullptr);
        ParseResult<StructDecl>
        parse_StructDecl(const std::shared_ptr<__TOKEN_N::TokenList> &modifiers = nullptr);
        ParseResult<ConstDecl>
        parse_ConstDecl(const std::shared_ptr<__TOKEN_N::TokenList> &modifiers = nullptr);
        ParseResult<ClassDecl> parse_ClassDecl(const std::shared_ptr<__TOKEN_N::TokenList> &modifiers = nullptr);
        ParseResult<ExtendDecl> parse_ExtendDecl(const std::shared_ptr<__TOKEN_N::TokenList> &modifiers = nullptr);
        ParseResult<InterDecl>
        parse_InterDecl(const std::shared_ptr<__TOKEN_N::TokenList> &modifiers = nullptr);
        ParseResult<EnumDecl>
        parse_EnumDecl(const std::shared_ptr<__TOKEN_N::TokenList> &modifiers = nullptr);
        ParseResult<TypeDecl>
        parse_TypeDecl(const std::shared_ptr<__TOKEN_N::TokenList> &modifiers = nullptr);

        ParseResult<FuncDecl>
        parse_FuncDecl(const std::shared_ptr<__TOKEN_N::TokenList> &modifiers  = nullptr,
                       bool                                         force_name = true);

        ParseResult<VarDecl> parse_VarDecl(bool force_type = false, bool force_value = false);
        ParseResult<FFIDecl>
        parse_FFIDecl(const std::shared_ptr<__TOKEN_N::TokenList> &modifiers = nullptr);
        ParseResult<LetDecl>
        parse_LetDecl(const std::shared_ptr<__TOKEN_N::TokenList> &modifiers = nullptr);
        ParseResult<OpDecl>
        parse_OpDecl(const std::shared_ptr<__TOKEN_N::TokenList> &modifiers = nullptr);
    };
}  //  namespace __AST_NODE_BEGIN

#endif  // __AST_BASE_DECLARATION_H__