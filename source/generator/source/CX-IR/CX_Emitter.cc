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

void generator::CXIR::CXIR::visit(__AST_NODE::Program &node) {
    std::erase_if(node.children, [&](const auto &child) {
        if (child->getNodeType() == __AST_NODE::nodes::FFIDecl) {
            child->accept(*this);
            return true;
        }
        return false;
    });

    std::string _namespace = sanitize_string(node.get_file_name());

    error::NAMESPACE_MAP[_namespace] =
        sanitize_string(std::filesystem::path(node.get_file_name()).stem().generic_string());

    // insert header guards
    ADD_TOKEN(CXX_PP_IFNDEF);
    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, _namespace + "_M");
    ADD_TOKEN(CXX_PP_DEFINE);
    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, _namespace + "_M");

    ADD_TOKEN(CXX_NAMESPACE);

    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, "helix");
    ADD_TOKEN(CXX_SCOPE_RESOLUTION);
    ADD_TOKEN_AS_VALUE(CXX_CORE_IDENTIFIER, _namespace);

    ADD_TOKEN(CXX_LBRACE);

    std::vector<__AST_N::NodeT<__AST_NODE::FuncDecl>> main_funcs;

    std::for_each(node.children.begin(), node.children.end(), [&](const auto &child) {
        if (child->getNodeType() == __AST_NODE::nodes::FuncDecl) {
            __AST_N::NodeT<__AST_NODE::FuncDecl> func = __AST_N::as<__AST_NODE::FuncDecl>(child);
            auto                                 name = func->get_name_t();

            if (!name.empty() && name.size() == 1) {
                /// all allowed main functions in all platforms of c++ are:
                /// main, wmain, WinMain, wWinMain, _tmain, _tWinMain

                if (name[0].value() == "main" || name[0].value() == "_main" ||
                    name[0].value() == "wmain" || name[0].value() == "WinMain" ||
                    name[0].value() == "wWinMain" || name[0].value() == "_tmain" ||
                    name[0].value() == "_tWinMain" ||
                    (!node.entry.empty() && name[0].value() == node.entry)) {
                    if (node.entry.empty() || name[0].value() != node.entry) {
                        /// there must be a return type
                        if (func->returns == nullptr) {
                            CODEGEN_ERROR(name[0], "main functions must have a return type (i32)");
                        }
                    }

                    auto &func_body = func->body->body->body;

                    /// insert an __inline_cpp("using namespace helix::_namespace;") at the start of
                    /// the main function
                    auto make_token = [&node](token::tokens kind, const std::string &value) {
                        return token::Token(kind, node.get_file_name(), value);
                    };

                    token::TokenList inline_cpp = {
                        make_token(token::tokens::IDENTIFIER, "__inline_cpp"),
                        make_token(token::tokens::PUNCTUATION_OPEN_PAREN, "("),
                        make_token(token::tokens::LITERAL_STRING,
                                   "\"using namespace helix; using namespace helix:: " +
                                       _namespace + "\""),
                        make_token(token::tokens::PUNCTUATION_CLOSE_PAREN, ")"),
                        make_token(token::tokens::PUNCTUATION_SEMICOLON, ";"),
                        make_token(token::tokens::EOF_TOKEN, "")};

                    token::TokenList::TokenListIter inline_cpp_iter = inline_cpp.begin();

                    __AST_NODE::Statement                       parser(inline_cpp_iter);
                    __AST_N::ParseResult<__AST_NODE::ExprState> inline_cpp_node =
                        parser.parse<__AST_NODE::ExprState>();

                    func_body.insert(func_body.begin(), inline_cpp_node.value());
                    main_funcs.push_back(func);

                    return;
                }
            }
        }

        child->accept(*this);
    });

    tokens.push_back(std::make_unique<CX_Token>(cxir_tokens::CXX_RBRACE));

    for (auto &func : main_funcs) {
        func->accept(*this);
    }

    tokens.push_back(std::make_unique<CX_Token>(cxir_tokens::CXX_CORE_IDENTIFIER, "#endif"));
}
