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

#include "generator/include/CX-IR/loc.hh"
#include "neo-panic/include/error.hh"
#include "neo-types/include/hxint.hh"

#include "utils.hh"

__CXIR_CODEGEN_BEGIN {
    std::string normalize_file_name(std::string file_name) {
        if (file_name.empty()) {
            return "";
        }

        if (file_name == "_H1HJA9ZLO_17.helix-compiler.cxir" || file_name == "helix_internal.file") {
            file_name.clear();
            file_name = "";
        }

        return file_name;
    }

    void handle_file_change(std::string & file_name,
                            std::string & cxir,
                            const std::unique_ptr<CX_Token> &token,
                            SourceMap                       &source_map,
                            size_t                          &cxir_line,
                            size_t                          &cxir_col) {

        if (normalize_file_name(file_name) == normalize_file_name(token->get_file_name())) {
            return;
        }

        if (token && token->get_line() != 0 &&
            !(normalize_file_name(token->get_file_name()).empty())) {

            file_name = token->get_file_name();
            if (file_name.empty()) {
                return;
            }

            cxir += "\n#line 1 R\"(" + file_name + ")\"\n";

            if (SOURCE_MAPS.find(file_name) == SOURCE_MAPS.end()) {
                SOURCE_MAPS[file_name] = SourceMap();
            }

            source_map = SOURCE_MAPS[file_name];

            // reset the line and column
            cxir_line = 1;
            cxir_col  = 1;
        }
    }

    std::string CXIR::generate_CXIR() const {
        /// goals:
        /// 1. we have to genrate the first #line directive to point to the original file and line 1
        /// 2. everey CXIR token has to be separated by a newline (as to keep track of the excat
        /// position in the original file)
        ///    - this is important for the source map generation later on
        /// 3. some CXIR tokens may not have any location information, in this case we have to look
        /// ahead and behind to find the closest token with location information
        ///    - in the case the diffrerence on LL and LR is the same, we choose LR (right) since it
        ///    is more likely to be the correct one UNLESS theres a semicolon in between current and
        ///    LR, in this case we choose LL

        size_t      cxir_line = 1;
        size_t      cxir_col  = 1;
        std::string cxir;
        std::string file_name;

        // get the first file name
        for (const auto &token : tokens) {
            if (token && token->get_line() != 0 &&
                !(normalize_file_name(token->get_file_name()).empty())) {
                file_name = token->get_file_name();
                cxir += "\n#line 1 R\"(" + file_name + ")\"\n";
                break;
            }
        }

        SOURCE_MAPS[file_name] = SourceMap();
        SourceMap &source_map  = SOURCE_MAPS[file_name];

        // now we have to generate the CXIR code
        for (size_t i = 0; i < tokens.size(); i++) {
            const auto &token = tokens[i];

            if (!token || token->get_value().empty()) {
                continue;
            }

            if ((token->get_value() == "#endif" ||
                 token->get_type() == cxir_tokens::CXX_PP_ENDIF)) {
                cxir += "\n" + token->get_value() + "\n";

                cxir_col = 1;
                source_map.append({.line   = token->get_line(),
                                   .column = token->get_column(),
                                   .length = token->get_length()},
                                  cxir_line++,
                                  cxir_col);
                ++cxir_line;

                continue;
            }

            if ((token->get_value() == "#include" ||
                 token->get_type() == cxir_tokens::CXX_PP_INCLUDE) ||
                (token->get_value() == "#define" ||
                 token->get_type() == cxir_tokens::CXX_PP_DEFINE) ||
                (token->get_value() == "#ifdef" ||
                 token->get_type() == cxir_tokens::CXX_PP_IFDEF) ||
                (token->get_value() == "#ifndef" ||
                 token->get_type() == cxir_tokens::CXX_PP_IFNDEF) ||
                (token->get_value() == "#pragma" ||
                 token->get_type() == cxir_tokens::CXX_PP_PRAGMA) ||
                (token->get_value() == "#undef" ||
                 token->get_type() == cxir_tokens::CXX_PP_UNDEF)) {

                if ((i + 1) < tokens.size()) {
                    cxir += "\n" + token->get_value() + " " + tokens[i + 1]->get_value() + "\n";
                } else {
                    continue;
                }

                ++i;  // skip the next token

                cxir_col = 1;  // reset the column number

                source_map.append({.line   = token->get_line(),
                                   .column = token->get_column(),
                                   .length = token->get_length()},
                                  cxir_line++,
                                  cxir_col);

                ++cxir_line;  // increment the line number (for the newline)
                continue;
            }

            SourceLocation source_mapped_token = {.line   = token->get_line(),
                                                  .column = token->get_column(),
                                                  .length = token->get_length()};

            // find the nearest valid token
            if (token->get_line() == 0) {
                size_t LL = SIZE_MAX;  // left token index
                size_t LR = SIZE_MAX;  // left token index
                size_t NI = SIZE_MAX;  // nearest token index

                for (size_t j = i; j < tokens.size(); j++) {  // find the nearest token on the right
                    if (tokens[j]->get_line() != 0) {
                        LR = j;
                        break;
                    }
                }

                for (size_t j = i; j > 0; j--) {  // find the nearest token on the left
                    if (tokens[j]->get_line() != 0) {
                        LL = j;
                        break;
                    }
                }

                if (LL == SIZE_MAX && LR == SIZE_MAX) {
                    source_mapped_token = {.line           = 0,
                                           .column         = 0,
                                           .length         = token->get_length(),
                                           .is_placeholder = true};

                    goto APPEND_TOKEN_TO_CXIR;
                }

                if (LL == SIZE_MAX) {
                    NI = LR;
                } else if (LR == SIZE_MAX) {
                    NI = LL;
                } else {
                    if (LR - i == i - LL) {
                        // if the difference is the same, we choose LR unless there is a semicolon
                        // in between current and LR

                        bool semicolon_found = false;

                        for (size_t j = i; j < LR; j++) {
                            if (tokens[j]->get_value() == ";" ||
                                tokens[j]->get_type() == cxir_tokens::CXX_SEMICOLON) {
                                semicolon_found = true;
                                break;
                            }
                        }

                        if (semicolon_found) {
                            NI = LL;
                        } else {
                            NI = LR;
                        }
                    } else if (LR - i < i - LL) {  // choose the nearest one
                        NI = LR;
                    } else {
                        NI = LL;
                    }
                }

                if (NI == SIZE_MAX) {
                    source_mapped_token = {.line   = tokens[NI]->get_line(),
                                           .column = tokens[NI]->get_column(),
                                           .length = tokens[NI]->get_length()};
                } else {
                    if (LR != SIZE_MAX && LR != 0) {
                        source_mapped_token = {.line   = tokens[LR]->get_line(),
                                               .column = tokens[LR]->get_column(),
                                               .length = tokens[LR]->get_length()};
                    } else if (LL != SIZE_MAX && LL != 0) {
                        source_mapped_token = {.line   = tokens[LL]->get_line(),
                                               .column = tokens[LL]->get_column(),
                                               .length = tokens[LL]->get_length()};
                    } else {
                        source_mapped_token = {.line   = 0,
                                               .column = 0,
                                               .length = token->get_length(),
                                               .is_placeholder = true};
                    }
                }
            }

        APPEND_TOKEN_TO_CXIR:
            handle_file_change(file_name, cxir, token, source_map, cxir_line, cxir_col);
            source_map.append(source_mapped_token, cxir_line, cxir_col);

            cxir += token->get_value() + " ";

            if (cxir_col > 1000) {  // fill upto 1000 columns (this is for more efficient source map
                                    // generation)
                cxir_col = 1;

                cxir += "\n";
                ++cxir_line;
            } else {
                cxir_col += (token->get_length() != 1 ? token->get_length() : token->get_value().length()) + 1;
            }
        }

        return cxir;
    }

}  // namespace __CXIR_CODEGEN_END
