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

// _H_RESERVED

#ifndef __CXIR_RESERVED__
#define __CXIR_RESERVED__

#include <functional>
#include <map>
#include <string>

#include "generator/include/CX-IR/CXIR.hh"
#include "generator/include/config/Gen_config.def"
#include "neo-pprint/include/hxpprint.hh"
#include "token/include/Token.hh"

__CXIR_CODEGEN_BEGIN {
    /// this contains a map with a bunch of lambdas on what changes what
    using TransformMap =
        std::map<std::string, std::function<void(CXIR *, const __TOKEN_N::Token &)>>;

    void basic_name_transform(CXIR *self, const __TOKEN_N::Token &token) {
        self->append(std::make_unique<CX_Token>(CXX_CORE_IDENTIFIER, "_H_RESERVED$" + token.value(), token));
    }

    TransformMap reserved_transformations{
        {"self",
         [](CXIR *self, const __TOKEN_N::Token &token) {
             self->append(CXX_LPAREN);
             self->append(CXX_ASTERISK);
             self->append(CXX_THIS);
             self->append(CXX_RPAREN);
         }},

        {"short",       &basic_name_transform},
        {"static",      &basic_name_transform},
        {"using",       &basic_name_transform},
        {"double",      &basic_name_transform},
        {"float",       &basic_name_transform},
        {"long",        &basic_name_transform},
        {"unsigned",    &basic_name_transform},
        {"signed",      &basic_name_transform},
        {"char",        &basic_name_transform},
        {"new",         &basic_name_transform},
        {"new",         &basic_name_transform},
        {"int",         &basic_name_transform}  //
    };
}  // namespace __CXIR_CODEGEN_END

#endif  // __CXIR_RESERVED__