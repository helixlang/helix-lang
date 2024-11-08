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

#include "parser/preprocessor/include/preprocessor.hh"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#include "controller/include/tooling/tooling.hh"
#include "neo-panic/include/error.hh"
#include "parser/ast/include/AST.hh"
#include "parser/ast/include/config/AST_config.def"
#include "parser/ast/include/nodes/AST_expressions.hh"
#include "parser/ast/include/nodes/AST_statements.hh"
#include "parser/preprocessor/include/private/dependency_tree.hh"
#include "token/include/Token.hh"
#include "token/include/private/Token_base.hh"
#include "token/include/private/Token_generate.hh"

// parser::preprocessor::import_tree

#define ADVANCE_AND_CHECK          \
    iter.advance();                \
    if (iter.remaining_n() == 0) { \
        continue;                  \
    }

std::vector<std::thread> THREAD_POOL;

class ImportProcessor {
    using SingleImportNormalized =
        std::tuple<std::variant<std::filesystem::path, token::TokenList>, token::TokenList, bool>;

    using MultipleImportsNormalized = std::vector<SingleImportNormalized>;

    token::TokenList                  &tokens;
    std::vector<std::filesystem::path> import_dirs;

    ImportProcessor(token::TokenList &tokens, std::vector<std::filesystem::path> &import_dirs)
        : tokens(tokens)
        , import_dirs(import_dirs) {}

    static token::TokenList
    normalize_scope_path(const parser::ast::NodeT<parser::ast::node::ScopePathExpr> &scope,
                         token::Token                                                start_tok) {
        token::TokenList final_path;

        if (scope->global_scope) {
            // handle special import cases
            // TODO

            throw error::Panic(error::CodeError{
                .pof      = &start_tok,
                .err_code = 0.0001,
                .mark_pof = true,
                .fix_fmt_args{},
                .err_fmt_args{GET_DEBUG_INFO + "global import not yet supported'"},
                .opt_fixes{},
            });
        }

        if (!scope->path.empty()) {
            for (auto &ident : scope->path) {
                final_path.emplace_back(ident->name);
            }
        }

        if (scope->access->getNodeType() != parser::ast::node::nodes::IdentExpr ||
            scope->access == nullptr) {

            if (!final_path.empty()) {
                throw error::Panic(error::CodeError{
                    .pof      = &final_path.back(),
                    .err_code = 0.0001,
                    .mark_pof = true,
                    .fix_fmt_args{},
                    .err_fmt_args{"expected an identifier after '::', but found something else"},
                    .opt_fixes{},
                });
            }

            throw std::runtime_error(GET_DEBUG_INFO + "invalid path access");
        }

        final_path.emplace_back(
            std::static_pointer_cast<parser::ast::node::IdentExpr>(scope->access)->name);

        return final_path;
    }

    /// \return a tuple containing the resolved path, the alias, and a bool indicating if the import
    /// is a wildcard
    static SingleImportNormalized
    resolve_single_import(const parser::ast::NodeT<parser::ast::node::SingleImport> &single_import,
                          token::Token                                               start_tok) {
        token::TokenList final_path;
        token::TokenList alias;

        // handle the alias
        if (single_import->alias != nullptr) {
            if (single_import->is_wildcard) {
                throw error::Panic(error::CodeError{
                    .pof      = &start_tok,
                    .err_code = 0.0001,
                    .mark_pof = true,
                    .fix_fmt_args{},
                    .err_fmt_args{"wildcard imports cannot have an alias"},
                    .opt_fixes{},
                });
            }

            if (single_import->alias->getNodeType() == parser::ast::node::nodes::IdentExpr) {
                alias.emplace_back(
                    std::static_pointer_cast<parser::ast::node::IdentExpr>(single_import->alias)
                        ->name);
            } else if (single_import->alias->getNodeType() ==
                       parser::ast::node::nodes::ScopePathExpr) {
                /// TODO
                // parser::ast::NodeT<parser::ast::node::ScopePathExpr> path =
                //     std::static_pointer_cast<parser::ast::node::ScopePathExpr>(
                //         single_import->alias);
                //
                // alias = normalize_scope_path(path, start_tok);
            } else {
                throw error::Panic(error::CodeError{
                    .pof      = &start_tok,
                    .err_code = 0.0001,
                    .mark_pof = true,
                    .fix_fmt_args{},
                    .err_fmt_args{"expected an identifier or a scope path for the alias, but found "
                                  "something else"},
                    .opt_fixes{},
                });
            }
        }

        // handle the path
        if (single_import->type == parser::ast::node::SingleImport::Type::Module) {
            if (single_import->path->getNodeType() != parser::ast::node::nodes::ScopePathExpr) {
                throw error::Panic(error::CodeError{
                    .pof      = &start_tok,
                    .err_code = 0.0001,
                    .mark_pof = true,
                    .fix_fmt_args{},
                    .err_fmt_args{"expected a scope path for the import, but found something else"},
                    .opt_fixes{},
                });
            }

            parser::ast::NodeT<parser::ast::node::ScopePathExpr> path =
                std::static_pointer_cast<parser::ast::node::ScopePathExpr>(single_import->path);

            final_path = normalize_scope_path(path, start_tok);
            return {final_path, alias, single_import->is_wildcard};
        }

        if (single_import->type == parser::ast::node::SingleImport::Type::File) {
            if (single_import->path->getNodeType() != parser::ast::node::nodes::LiteralExpr) {
                throw error::Panic(error::CodeError{
                    .pof      = &start_tok,
                    .err_code = 0.0001,
                    .mark_pof = true,
                    .fix_fmt_args{},
                    .err_fmt_args{
                        "expected a string literal for the import, but found something else"},
                    .opt_fixes{},
                });
            }

            std::filesystem::path f_path =
                std::static_pointer_cast<parser::ast::node::LiteralExpr>(single_import->path)
                    ->value.value();
            return {f_path, alias, single_import->is_wildcard};
        }

        throw std::runtime_error(GET_DEBUG_INFO + "invalid import type");
    }

    static MultipleImportsNormalized
    resolve_spec_import(const parser::ast::NodeT<parser::ast::node::SpecImport> &spec_import,
                        token::Token                                             start_tok) {
        token::TokenList base_path = normalize_scope_path(spec_import->path, std::move(start_tok));
        MultipleImportsNormalized imports;

        if (spec_import->type == parser::ast::node::SpecImport::Type::Wildcard) {
            if (spec_import->imports == nullptr) {
                throw error::Panic(error::CodeError{
                    .pof      = &start_tok,
                    .err_code = 0.0001,
                    .mark_pof = true,
                    .fix_fmt_args{},
                    .err_fmt_args{"expected a wildcard import, but found a symbol import"},
                    .opt_fixes{},
                });
            }

            imports.emplace_back(base_path, token::TokenList{}, true);
        } else if (spec_import->type == parser::ast::node::SpecImport::Type::Symbol) {
            if (spec_import->imports == nullptr || spec_import->imports->imports.empty()) {
                throw error::Panic(error::CodeError{
                    .pof      = &start_tok,
                    .err_code = 0.0001,
                    .mark_pof = true,
                    .fix_fmt_args{},
                    .err_fmt_args{"expected at least one import item, but found none"},
                    .opt_fixes{},
                });
            }

            if (!spec_import->imports->imports.empty()) {
                for (auto &import : spec_import->imports->imports) {
                    auto [path, alias, is_wildcard] = resolve_single_import(import, start_tok);

                    /// append the base path to the start of the single import path
                    std::get<token::TokenList>(path).insert(
                        std::get<token::TokenList>(path).cbegin(),
                        base_path.cbegin(),
                        base_path.cend());

                    imports.emplace_back(path, alias, is_wildcard);
                }
            } else {
                throw error::Panic(error::CodeError{
                    .pof      = &start_tok,
                    .err_code = 0.0001,
                    .mark_pof = true,
                    .fix_fmt_args{},
                    .err_fmt_args{"expected at least one import item, but found none"},
                    .opt_fixes{},
                });
            }

            return imports;
        }
    }

    auto insert_inline_cpp(
        token::TokenList                                                     &tokens,
        const token::Token                                                   &loc,
        const std::variant<std::pair<std::string, std::string>, std::string> &cxx) -> void {

        token::TokenList inline_cpp;

        auto make_token = [&loc](const std::string &str, token::tokens kind) -> token::Token {
            return {loc.line_number(),
                    loc.column_number(),
                    str.length(),
                    loc.offset() + str.length(),
                    str,
                    loc.file_name(),
                    token::tokens_map.at(kind).value()};
        };

        inline_cpp.push_back(make_token("__inline_cpp", token::tokens::IDENTIFIER));
        inline_cpp.push_back(make_token("(", token::tokens::PUNCTUATION_OPEN_PAREN));

        if (cxx.index() == 0) {
            auto [path, alias] = std::get<std::pair<std::string, std::string>>(cxx);
            inline_cpp.push_back(make_token("\"namespace " + path + " = " + alias + ";\"",
                                            token::tokens::LITERAL_STRING));
        } else {
            inline_cpp.push_back(
                make_token("\"using namespace " + std::get<std::string>(cxx) + ";\"",
                           token::tokens::LITERAL_STRING));
        }

        inline_cpp.push_back(make_token(")", token::tokens::PUNCTUATION_CLOSE_PAREN));
        inline_cpp.push_back(make_token(";", token::tokens::PUNCTUATION_SEMICOLON));
    }

    void parse() {
        /// make an ast parser
        token::TokenList::TokenListIter                          iter = tokens.begin();
        parser::ast::node::Statement                             ast_parser(iter);
        parser::ast::NodeT<parser::ast::node::ImportState>       import;
        parser::ast::ParseResult<parser::ast::node::ImportState> import_result;

        size_t       start_pos = iter.position();
        token::Token start     = iter.current();
        token::Token end;
        bool         found_import = false;

        while (iter.remaining_n() > 0) {
            if (iter->token_kind() == token::tokens::KEYWORD_FFI) {
                /// TODO: add checks to see if tokens are empty after advancing
                bool         has_brace = false;
                token::Token ffi_tok   = iter.current();

                ADVANCE_AND_CHECK;  // skip 'ffi'

                if (iter->token_kind() != token::tokens::LITERAL_STRING) {
                    token::Token bad_tok = iter.current();

                    throw error::Panic(error::CodeError{
                        .pof      = &bad_tok,
                        .err_code = 0.0001,
                        .mark_pof = true,
                        .fix_fmt_args{},
                        .err_fmt_args{"expected a string literal after 'ffi'"},
                        .opt_fixes{},
                    });
                }

                ADVANCE_AND_CHECK;  // skip the language

                if (iter->token_kind() == token::tokens::PUNCTUATION_OPEN_BRACE) {
                    has_brace = true;
                    ADVANCE_AND_CHECK;  // skip the opening brace
                }

                if (iter->token_kind() != token::tokens::KEYWORD_IMPORT) {
                    continue;  // skip parsing the ffi block if it's not an import
                }

                while (iter->token_kind() != (has_brace ? token::tokens::PUNCTUATION_CLOSE_BRACE
                                                        : token::tokens::PUNCTUATION_SEMICOLON)) {
                    if (iter.remaining_n() == 0) {
                        throw error::Panic(error::CodeError{
                            .pof      = &ffi_tok,
                            .err_code = 0.0001,
                            .mark_pof = true,
                            .fix_fmt_args{},
                            .err_fmt_args{std::string("expected a") +
                                          (has_brace ? " '}'" : " ';'") +
                                          " to close the 'ffi' block"},
                            .opt_fixes{},
                        });
                    }

                    ADVANCE_AND_CHECK;  // WARNING: this may fail since we break and dont break
                                        // again.
                }
            }

            if (iter->token_kind() == token::tokens::KEYWORD_IMPORT) {
                /// remove all import tokens
                /// calculate the 'import' token to the ';' or '}' token
                u64  offset    = 1;
                bool has_brace = false;

                if (!iter.peek(offset).has_value()) {
                    throw error::Panic(error::CodeError{
                        .pof      = &start,
                        .err_code = 0.0001,
                        .mark_pof = true,
                        .fix_fmt_args{},
                        .err_fmt_args{"expected a something after 'import' keyword"},
                        .opt_fixes{},
                    });
                }

                if (iter.peek(offset)->get().token_kind() ==
                    token::tokens::PUNCTUATION_OPEN_BRACE) {
                    has_brace = true;
                    ++offset;  // skip the opening brace
                }

                while (iter.peek(offset).has_value()) {  // at this stage this is safe
                    if (!iter.peek(offset).has_value()) {
                        throw error::Panic(error::CodeError{
                            .pof      = &start,
                            .err_code = 0.0001,
                            .mark_pof = true,
                            .fix_fmt_args{},
                            .err_fmt_args{"expected a ';' or '}' to close the 'import' statement"},
                            .opt_fixes{},
                        });
                    }

                    if (iter.peek(offset)->get().token_kind() ==
                        (has_brace ? token::tokens::PUNCTUATION_CLOSE_BRACE
                                   : token::tokens::PUNCTUATION_SEMICOLON)) {
                        end = iter.peek(offset).value().get();
                        break;
                    }

                    ++offset;
                }

                import_result = ast_parser.parse<parser::ast::node::ImportState>();
                found_import  = true;
                break;
            }

            iter.advance();
        }

        if (!found_import) {
            return;
        }

        /// remove the import tokens
        tokens.remove(start, end);

        if (!import_result.has_value()) {
            import_result.error().panic();
            return;
        }

        /// parse import if it exists
        import = import_result.value();

        /// NOTE: the first element of import_dirs is the cwd

        /// both spec and single imports are allowed here, and do get processed
        /// order of checking is important

        /// we first normalize the ... by doing the following: we take the whole scope path replace
        /// the `::` with `/` we dont add anything to the end tho just yet.

        /// if we have `import module ...` then we attach a .hlx to the end of it and then resolve
        /// it starting with the cwd then each of the specified import paths, if we have collisions,
        /// we take what we see first then warn saying import also matched in other locations.

        /// if we have 'import ...` we first see if the specified dir exists, if so then does it
        /// contain a .hlx file matching the back of the path name, like lets say `import foo::bar`
        /// does foo/bar exist as a dir, if so does foo/bar/bar.hlx exist, if so process that as a
        /// whole different compile unit entirely since its a lib and follow external linkage
        /// but if foo/bar is not a dir then check for foo/bar.hlx, if it exists start a new compile
        /// action and do internal linkage.

        /// NOTE: if we have a spec import, backtrack each path from right to left looking, since
        ///       we cant resolve symbols yet and warn saying specified imported item will not get
        ///       added to the current namespace in this version of the compiler.

        /// if we have `import "..."` we directly look for the specified file, and apply external
        /// linkage

        /// if we have `import module ".."` we directly look for the specified file, and apply
        /// internal linkage

        /// after all this if we cant find any path error out.

        std::vector<std::filesystem::path> resolved_paths;
        MultipleImportsNormalized          resolved_imports;

        if (import->type == parser::ast::node::ImportState::Type::Single) {
            SingleImportNormalized imports = resolve_single_import(
                std::static_pointer_cast<parser::ast::node::SingleImport>(import->import), start);
            
            resolved_imports.emplace_back(imports);

        } else if (import->type == parser::ast::node::ImportState::Type::Spec) {
            auto imports = resolve_spec_import(
                std::static_pointer_cast<parser::ast::node::SpecImport>(import->import), start);
        
            resolved_imports.insert(resolved_imports.end(), imports.begin(), imports.end());
        }

        token::Token &marker = start;


        /// now we can process all the imports individually

        for (auto &import : resolved_imports) {
            
        }

        /// if we have a spec import, backtrack each path from right to left looking, since

        // /// spawn a thread to do the compilation unit over the resolved path
        // THREAD_POOL.emplace_back([=]() {
        //     // do the compilation unit
        // });
    }
};
