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

#ifndef __TOKEN_BASE_HH__
#define __TOKEN_BASE_HH__

#include <iostream>
#include <string>
#include <string_view>

#include "neo-json/include/json.hh"
#include "neo-types/include/hxint.hh"
#include "token/include/config/Token_config.def"
#include "token/include/private/Token_generate.hh"

__TOKEN_BEGIN {
    class TokenList;

    /*
    Token(u64 line, u64 column, u64 length, u64 offset, std::string_view value,
              const std::string &filename, std::string_view token_kind = "");
    */
    struct Token {
      private:
        mutable u32         line{};     ///< line number where the token is located
        mutable u32         column{};   ///< column number where the token starts
        u32                 len{};      ///< length of the token
        u32                 _offset{};  ///< offset from the beginning of the file
        tokens              kind{};     ///< kind of the token
        mutable std::string val;        ///< string value of the token
        std::string         filename;   ///< name of the file

      public:
        Token(u64                line,
              u64                column,
              u64                length,
              u64                offset,
              std::string_view   value,
              const std::string &filename,
              std::string_view   token_kind = "");

        Token(const Token &other);
        Token &operator=(const Token &other);
        Token(Token &&other) noexcept;
        Token &operator=(Token &&other) noexcept;
        Token &operator=(const std::string &other);
        Token();

        explicit Token(tokens token_type, const std::string &filename, std::string value = "");
        ~Token();

        /* ====-------------------------- getters ---------------------------==== */
        u32                        line_number() const;
        u32                        column_number() const;
        u32                       &get_column_number() const;
        u32                        length() const;
        u32                        offset() const;
        tokens                     token_kind() const;
        std::string                value() const;
        [[nodiscard]] std::string &get_value() const;
        std::string                token_kind_repr() const;
        std::string                file_name() const;
        std::string                to_string() const;

        bool          operator==(const Token &rhs) const;
        bool          operator==(const tokens &rhs) const;
        std::ostream &operator<<(std::ostream &os) const;
        Token        &operator+(const string &str);

        TO_NEO_JSON_IMPL {
            neo::json token_json("Token");

            token_json.add("length", len).add("kind", token_kind_repr()).add("value", val);

            neo::json &loc_sec = token_json.section("loc");

            loc_sec.add("filename", filename)
                .add("line_number", line)
                .add("column_number", column)
                .add("offset", _offset);

            return token_json;
        }

        /* ====-------------------------- setters ---------------------------==== */

        void set_file_name(const std::string &file_name);
        void set_value(const std::string &other);

        enum class OffsetType {
            Line,
            Colum,
            Offset,
        };

        void offset(OffsetType ty, u64 by);
    };

    Token bare_token(tokens token_type, std::string value = "");
}  // __TOKEN_BEGIN

__TOKEN_N::Token operator+(const std::string &lhs, __TOKEN_N::Token rhs);
__TOKEN_N::Token operator+(__TOKEN_N::Token &lhs, const std::string &rhs);

#endif  // __TOKEN_BASE_HH__
