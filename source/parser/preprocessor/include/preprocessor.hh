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

#ifndef __PRE_HH__
#define __PRE_HH__

#include <cstddef>
#include <filesystem>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "parser/preprocessor/include/dependency_tree.hh"
#include "token/include/Token.hh"
#include "token/include/private/Token_generate.hh"

/* best way to parse all the things we need to keep track of

- if module / inline module is found
    - we need now start keeping track of braces and namespaces
    - after we reach a missmatch in the number of braces compared to the number of namespaces, we continue counting but mark the index where the namespace ended



*/

struct Module {
    __TOKEN_N::Token name;
    size_t index;
};

namespace parser::preprocessor {
using QualifiedName      = __TOKEN_N::TokenList;
using IncludeDirectories = std::list<std::filesystem::path>;
using OptToken           = std::optional<__TOKEN_N::Token>;
using MacroParamList = std::map<__TOKEN_N::Token,  __TOKEN_N::TokenList>;

struct MacroSymbol {
    // alias for parameter list with default args map<identifier, default arg>

    MacroParamList            params;  // parameters and default arguments
    __TOKEN_N::TokenList body;    // body of the macro
    QualifiedName        loc;     // namespace location something like `namespace::namespace::macro!` or (`macro!` with an an alias to '::macro!`)

    MacroSymbol() = default;
    MacroSymbol(MacroSymbol &&)                 = default;
    MacroSymbol(const MacroSymbol &)            = default;
    MacroSymbol &operator=(const MacroSymbol &) = delete;
    MacroSymbol &operator=(MacroSymbol &&)      = delete;

    ~MacroSymbol() = default;

    MacroSymbol(MacroParamList params, __TOKEN_N::TokenList body, QualifiedName loc)
        : params(std::move(params)), body(std::move(body)), loc(std::move(loc)) {}

    bool operator==(const MacroSymbol &other) const {
        return params == other.params && body == other.body && loc == other.loc;
    }

    bool operator!=(const MacroSymbol &other) const { return !(*this == other); }

    size_t number_of_params() const { return params.size(); }
    bool has_default_args() const {
        for (const auto &[_, default_arg] : params) {
            if (default_arg.size() > 0) {
                return true;
            }
        }
        return false;
    }
};

using MacroSymbolPtr = std::shared_ptr<MacroSymbol>;
using MacroInvokeLoc = std::pair<QualifiedName, __TOKEN_N::Token>;

class MacroMap {
  public:
    using MacroMap_t = std::map<__TOKEN_N::Token, MacroSymbolPtr>;

  private:
    MacroMap_t macros;

  public:
    MacroMap() = default;
    MacroMap(MacroMap &&)                 = default;
    MacroMap(const MacroMap &)            = default;
    MacroMap &operator=(const MacroMap &) = delete;
    MacroMap &operator=(MacroMap &&)      = delete;

    ~MacroMap() = default;

    void add_macro(const __TOKEN_N::Token &name, MacroSymbolPtr macro, MacroParamList params = {});
    void remove_macro(const __TOKEN_N::Token &name, size_t num_params = 0);
};


class Preprocessor {
  public:
    using AllowedABIOptions = std::array<string, __TOKEN_ABI_N::reserved.size()>;

    MacroMap           pp_macros;     // preprocessor macros
    AllowedABIOptions  allowed_abi;   // allowed ABI options
    IncludeDirectories include_dirs;  // directories for module inclusion

  private:
    using TokenIterator = __TOKEN_N::TokenList::TokenListIter;
    using BraceStack    = std::vector<i64>;

    __TOKEN_N::TokenList source_tokens;          // tokens from the source file
    QualifiedName        current_namespace;      // current namespace, taking nesting into account
    BraceStack           namespace_brace_level;  // pop when this is reached
    TokenIterator       *source_iter = nullptr;  // iterator over source tokens

    //===-------------------------------------- friends ---------------------------------------===//

    friend void parse_ffi(Preprocessor *);
    friend void parse_brace(Preprocessor *);
    friend void parse_macro(Preprocessor *);
    friend void parse_import(Preprocessor *);
    friend void parse_define(Preprocessor *);
    friend void parse_namespace(Preprocessor *);
    friend void parse_invocation(Preprocessor *);
    friend void handle_invalid_abi_option(Preprocessor *);

    //===------------------------------------ iter helpers ------------------------------------===//

    inline bool              is_source_iter_set() { return source_iter != nullptr; }
    inline __TOKEN_N::Token &current() { return source_iter->current().get(); }
    inline __TOKEN_N::Token &advance(const std::int32_t n = 1) {
        return source_iter->advance(n).get();
    }
    inline __TOKEN_N::Token &reverse(const std::int32_t n = 1) {
        return source_iter->reverse(n).get();
    }

    OptToken peek(const std::int32_t n = 1) const { return source_iter->peek(n)->get(); }
    OptToken peek_back(const std::int32_t n = 1) const { return source_iter->peek_back(n)->get(); }

    //===--------------------------------------------------------------------------------------===//

  public:
    explicit Preprocessor(__TOKEN_N::TokenList      &tokens,
                          const std::string         &name                = "",
                          const std::vector<string> &custom_include_dirs = {});
    Preprocessor(Preprocessor &&)                 = default;
    Preprocessor(const Preprocessor &)            = default;
    Preprocessor &operator=(const Preprocessor &) = delete;
    Preprocessor &operator=(Preprocessor &&)      = delete;

    ~Preprocessor();

    __TOKEN_N::TokenList
        parse(preprocessor::ImportNodePtr /* do NOT set when externally invoked */ = nullptr);
};

}  // namespace parser::preprocessor

#endif  // __PRE_HH__