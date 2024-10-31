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

#ifndef __TOKENS_HH__
#define __TOKENS_HH__

#include "token/include/config/Token_config.def"
#include "token/include/enums/Token_abi.def"
#include "token/include/enums/Token_delimiters.def"
#include "token/include/enums/Token_keywords.def"
#include "token/include/enums/Token_literals.def"
#include "token/include/enums/Token_operators.def"
#include "token/include/enums/Token_others.def"
#include "token/include/enums/Token_primitives.def"
#include "token/include/enums/Token_punctuation.def"
#include "token/include/types/mapping.hh"

// generate enum and maps for all tokens
GENERATE_RESERVED_ENUM_AND_MAPPING
GENERATE_TOKENS_ENUM_AND_MAPPING

// undefine the macros to prevent global namespace pollution
#undef MAKE_TOKEN
#undef MAKE_TOKEN_PAIR
#undef GENERATE_TOKENS_ENUM_AND_MAPPING

#undef KEYWORD_TOKENS_COUNT
#undef DELIMITER_TOKENS_COUNT
#undef LITERAL_TOKENS_COUNT
#undef OPERATOR_TOKENS_COUNT
#undef OTHER_TOKENS_COUNT
#undef PRIMITIVE_TOKENS_COUNT
#undef PUNCTUATION_TOKENS_COUNT

#undef TOKENS
#undef KEYWORD_TOKENS
#undef DELIMITER_TOKENS
#undef LITERAL_TOKENS
#undef OPERATOR_TOKENS
#undef OTHER_TOKENS
#undef PRIMITIVE_TOKENS
#undef PUNCTUATION_TOKENS

#endif  // __TOKENS_HH__