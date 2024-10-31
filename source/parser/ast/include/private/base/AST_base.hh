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

#ifndef __AST_BASE_H__
#define __AST_BASE_H__

#include <neo-pprint/include/hxpprint.hh>
#include <string>

#include "neo-pprint/include/ansi_colors.hh"
#include "parser/ast/include/config/AST_config.def"
#include "parser/ast/include/private/base/AST_base_declaration.hh"
#include "parser/ast/include/types/AST_types.hh"
#include "parser/ast/include/types/AST_visitor.hh"

__AST_NODE_BEGIN {
    class Node {  // base node
      public:
        Node()                                                                          = default;
        virtual ~Node()                                                                 = default;
        virtual void                      accept(__AST_VISITOR::Visitor &visitor) const = 0;
        [[nodiscard]] virtual nodes       getNodeType() const                           = 0;
        [[nodiscard]] virtual std::string getNodeName() const                           = 0;
        [[nodiscard]] virtual bool        is(nodes node) const                          = 0;
        template <typename T, typename U>
        [[nodiscard]] static NodeT<T> as(U &from) {
            return std::static_pointer_cast<T>(from);
        }

        Node(const Node &)            = default;
        Node &operator=(const Node &) = default;
        Node(Node &&)                 = default;
        Node &operator=(Node &&)      = default;
    };

    class Program : public Node {
      public:
        Program()                           = delete;
        ~Program() override                 = default;
        Program(const Program &)            = default;  // no move or copy semantics
        Program &operator=(const Program &) = delete;
        Program(Program &&)                 = delete;
        Program &operator=(Program &&)      = delete;

        explicit Program(__TOKEN_N::TokenList &source_tokens)
            : source_tokens(source_tokens) {}

        void accept(parser ::ast ::visitor ::Visitor &visitor) const override {
            visitor.visit(*this);
        }

        [[nodiscard]] nodes        getNodeType() const override { return nodes::Program; }
        [[nodiscard]] std ::string getNodeName() const override { return "Program"; };
        [[nodiscard]] bool         is(nodes node) const override { return node == nodes::Program; }

        Program &parse(bool quiet = false) {
            auto iter = source_tokens.begin();

            ParseResult<> expr;

            while (iter.remaining_n() != 0) {
                auto decl = node::Declaration(iter);
                expr      = decl.parse();

                if (!expr.has_value()) {
                    has_errored = true;
                    if (!quiet) { expr.error().panic();
}
#ifdef DEBUG
                    print(std::string(colors::fg16::red),
                          "error: ",
                          std::string(colors::reset),
                          expr.error().what());
#endif
                    return *this;
                }

                children.emplace_back(expr.value());
            }

            return *this;
        }

        NodeV<> children;
        NodeV<> annotations;
        bool has_errored = false;

      private:
        __TOKEN_N::TokenList &source_tokens;
    };
}  //  namespace __AST_NODE_BEGIN

#endif  // __AST_BASE_H__