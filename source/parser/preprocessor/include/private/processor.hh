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

#include "parser/preprocessor/include/private/dependency_tree.hh"
#include "token/include/Token.hh"
#include "token/include/private/Token_generate.hh"

/* best way to parse all the things we need to keep track of

- if module / inline module is found
    - we need now start keeping track of braces and namespaces
    - after we reach a mismatch in the number of braces compared to the number of namespaces, we continue counting but mark the index where the namespace ended
        - but now how to deal if we have a module def inside a brace that is not a namespace like this:
            module foo
            {
                inline module bar
                {
                    // code
                    {
                        module baz // disallowed since a module is not allowed in a local scope
                        {
                            // code
                        }
                    }
                }
            }


*/

struct Module {
    __TOKEN_N::Token name;
    size_t index;
};

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
        return std::ranges::any_of(params, [](const auto &param) {
            return param.second.size() > 0;
        });
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

#endif  // __PRE_HH__