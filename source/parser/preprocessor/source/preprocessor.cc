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

#include "parser/preprocessor/include/preprocessor.hh"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "neo-panic/include/error.hh"
#include "token/include/Token.hh"

namespace parser::preprocessor {
Preprocessor::Preprocessor(__TOKEN_N::TokenList      &tokens,
                           const std::string         &name,
                           const std::vector<string> &custom_include_dirs)
    : source_tokens(tokens) {

    if (!(tokens.size() > 0) || ((tokens.size() > 0) ? (tokens.size() - 1) : 0) == 0) [[unlikely]] {
        return;  // in the case that there are no tokens (physically impossible)
    }

    include_dirs.emplace_back(std::filesystem::path(tokens[0].file_name()).parent_path());

    for (const auto &path_str : custom_include_dirs) {
        include_dirs.emplace_back(path_str);
    }

    transform(__TOKEN_ABI_N::reserved.begin(),
              __TOKEN_ABI_N::reserved.end(),
              allowed_abi.begin(),
              [](const auto &pair) { return string(pair.second); });

    if (!name.empty() || name == "main") {
        preprocessor::import_tree = make_unique<preprocessor::ImportTree>(tokens[0].file_name());
    }
}

Preprocessor::~Preprocessor() {
    preprocessor::import_tree.reset();

    if (source_iter != nullptr) {
        source_iter = nullptr;
    }
}

void handle_invalid_abi_option(Preprocessor *self) {
    std::string abi_options;

    for (const std::string &abi : self->allowed_abi) {
        abi_options += abi + ", ";
    }

    if (!self->allowed_abi.empty()) {
        abi_options = abi_options.substr(0, abi_options.size() - 2);
    }

    auto bad_token = self->peek().value();
    error::Panic(
        error::CodeError{.pof = &bad_token, .err_code = 0.7004, .fix_fmt_args = {abi_options}});
}

void parse_import(Preprocessor *self) {}

/*
ffi "c++" import "some.hh";
ffi "py"  import "sympy";
          ^
ffi "py" {
         ^ or
    import "sympy";
    }

    ffi "py" {

    }
}
*/
void parse_ffi(Preprocessor *self) {  // at the time of call, current() is 'ffi'
    //     i32    brace_count = 0;
    //     bool   jump_back   = false;
    //     string goto_caller;

    //     auto advance_and_continue = [&]() {
    //         self->advance();
    //         jump_back = true;
    //     };

    // iter_body:
    //     switch (self->current().token_kind()) {  // at call time peek should be ffi
    //         case __TOKEN_TYPES_N::KEYWORD_FFI:
    //             if (self->peek().value_or(__TOKEN_N::Token()).token_kind() !=
    //                 __TOKEN_TYPES_N::LITERAL_STRING) {
    //                 error::Panic(error::CodeError{
    //                     .pof       = &self->current(),
    //                     .err_code  = 0.7005,
    //                     .mark_pof  = false,
    //                     .opt_fixes = {
    //                         {__TOKEN_N::bare_token(__TOKEN_TYPES_N::LITERAL_STRING, "\"...\""),
    //                          self->current().column_number() + self->current().length() + 1}}});
    //             }

    //             self->advance(2);  // skip ffi and the string ident

    //             if (self->current().token_kind() ==
    //                 __TOKEN_TYPES_N::KEYWORD_IMPORT) {  // ffi "c" import
    //                 goto_caller = "ffi";
    //                 goto abi_import;  // no loop back
    //             } else {              // if the import token is missing
    //                 auto bad_token = self->peek_back().value();
    //                 throw error::Panic(error::CodeError{
    //                     .pof = &bad_token, .err_code = 0.10001,
    //                     //.opt_fixes = {{__TOKEN_N::bare_token(__TOKEN_TYPES_N::KEYWORD_IMPORT) +
    //                     " ",
    //                     //-1}}
    //                 });
    //             }

    //             jump_back = true;
    //             break;

    //         case __TOKEN_TYPES_N::PUNCTUATION_OPEN_BRACE:  // ffi "py" { <-- this opening brace
    //             brace_count++;

    //             if (brace_count <= 0) {  // if there's more }'s than opening ones
    //                 throw error::Panic(error::CodeError{.pof          = &self->current(),
    //                                                     .err_code     = 2.1002,
    //                                                     .err_fmt_args = {"closing brace"}});
    //             }

    //             advance_and_continue();  // skip {
    //             break;

    //         case __TOKEN_TYPES_N::PUNCTUATION_CLOSE_BRACE:  // } <-- this closing brace
    //             brace_count--;

    //             if (brace_count < 0) {  // if there's an additional closing brace
    //                 throw error::Panic(error::CodeError{.pof          = &self->current(),
    //                                                     .err_code     = 2.1002,
    //                                                     .err_fmt_args = {"opening brace"}});
    //             }

    //             if (brace_count == 0) {  // completed
    //                 return;
    //             }

    //             advance_and_continue();  // skip }
    //             break;

    //         case __TOKEN_TYPES_N::KEYWORD_IMPORT:  // import <-- this "sympy";
    //         abi_import:
    //             // syntax: 'import' (string (',' string)*) ';'
    //             if (!self->peek().has_value()) {  // just 'import' no specifier
    //                 auto bad_token = self->peek().value();
    //                 throw error::Panic(error::CodeError{
    //                     .pof = &bad_token, .err_code = 0.10001, .err_fmt_args = {"opening
    //                     brace"}});
    //             }

    //             if (self->peek().value().token_kind() != __TOKEN_TYPES_N::LITERAL_STRING) {
    //                 // 0.10101
    //                 auto bad_token = self->peek().value();
    //                 throw error::Panic(error::CodeError{
    //                     .pof          = &bad_token,
    //                     .err_code     = 0.10101,
    //                     .fix_fmt_args = {"import \"" + self->peek().value().value() + "\""}});
    //             }

    //             self->advance(2);  // skip 'import' string | other

    //             if (self->current().token_kind() ==
    //                 __TOKEN_TYPES_N::PUNCTUATION_COMMA) {  // (',' string)*
    //                 // 0.10102
    //                 error::Panic(error::CodeError{.pof = &self->current(), .err_code = 0.10102});
    //                 // TODO: add parsing if needed (might not look cohesive)
    //                 //       don't forget to advance after parsing out multiple
    //             }

    //             // ';' at this point it should only be a semicolon
    //             if (self->current().token_kind() != __TOKEN_TYPES_N::PUNCTUATION_SEMICOLON) {
    //                 auto bad_token = self->peek_back().value();
    //                 error::Panic(error::CodeError{
    //                     .pof       = &bad_token,
    //                     .err_code  = 4.0001,
    //                     .mark_pof  = false,
    //                     .opt_fixes = {
    //                         {__TOKEN_N::bare_token(__TOKEN_TYPES_N::PUNCTUATION_SEMICOLON),
    //                         -1}}});
    //             }

    //             advance_and_continue();  // skip ; or ,

    //             if (goto_caller == "ffi") {
    //                 return;  // import completed
    //             }

    //             break;

    //         case __TOKEN_TYPES_N::PUNCTUATION_SEMICOLON:
    //             advance_and_continue();  // skip ;'s
    //             break;

    //         default:
    //             if (brace_count > 0) {
    //                 advance_and_continue();  // skip ;
    //                 break;
    //             } else {
    //                 error::Panic(error::CodeError{.pof = &self->current(), .err_code = 0.0001});
    //             }
    //     }

    //     if (jump_back) {
    //         jump_back = false;
    //         goto iter_body;
    //     }
}

void parse_define(Preprocessor *self) {
    // start a table of modules
    // module foo::bar {
    // }
    // remember that we are now in `foo` `::` `bar`

    // if a define is found here

    // store it in map<vec<tokens>, vec<tokens>> - or some similar structure
    // when a invocation is made, look back keep going back in the order of ident <- :: until the
    // order breaks look in the map for it or bring it down to most similar (like 90% matches) for
    // example if we are in module foo::bar::zoo::foo {
    //      // and have a define
    //      define PI!: 3.12819;
    // }
    // our table would have foo::bar::zoo::foo::PI
    // conversely:
    // if theres a define in the global scope
    // module {
    // define PI: 1020;
    // }
    // both: '::PI!' and 'PI!' should work
    // in a invocation, if we are in foo: we except to see foo::bar::zoo::foo::PI! or
    // bar::zoo::foo::PI! if theres something like zoo::foo::PI!, its not valid and we can suggest
    // to add bar:: to the line once we store the define in a structure we remove it from the
    // tokenList entirely, allowing for invocations to be parsed correctly.

    // work to do in parse_define:
    // keep track of current module scope
    // find a define, make sure it ends with };
    //      if not error out saying unclosed define
    // store the content of the define into the structure along with its location
    // also parse out number of params and any defaults
    // expose the structure into the compiler data set (allowing for complex introspection)


}

void parse_macro(Preprocessor *self) {}

// void __parse_namespace(Preprocessor *self) {  // at the time of call, current() is 'module'
//     // module foo::bar {
//     QualifiedName namespace_name;

//     auto check_next_token = [&]() {
//         if (!self->peek().has_value()) [[unlikely]] {
//             throw error::Panic(error::CodeError{
//                 .pof          = &self->current(),
//                 .err_code     = 2.1004,
//                 .mark_pof     = false,
//                 .err_fmt_args = {"opening brace"},

//                 // TODO: add space between insert if its not there
//                 //       and also make sure it clips to avoid inserting inside a color

//                 .opt_fixes = {{__TOKEN_N::bare_token(__TOKEN_TYPES_N::PUNCTUATION_OPEN_BRACE),
//                 -1}}});
//         }
//     };

//     check_next_token();
//     if (self->peek().value().token_kind() != __TOKEN_TYPES_N::IDENTIFIER) {
//         // this is a anonyms namespace (ignored for defines)
//         self->advance();  // module (:|{)<
//     } else {
//         // if this is a valid namespace

//         check_next_token();
//         if (self->current().token_kind() != __TOKEN_TYPES_N::IDENTIFIER) {
//             // if theres a diffrent token
//             throw error::Panic(error::CodeError{
//                 .pof          = &self->current(),
//                 .err_code     = 2.1004,
//                 .err_fmt_args = {"opening brace. instead encountered an unexecuted token"},

//                 // TODO: add space between insert if its not there
//                 //       and also make sure it clips to avoid inserting inside a color

//                 .opt_fixes = {{__TOKEN_N::bare_token(__TOKEN_TYPES_N::PUNCTUATION_OPEN_BRACE),
//                 -1}}});
//         }

//     error::Panic(
//                 error::CodeError{.pof      = &self->current(),
//                                 .err_code = 0.7008,

//                                 // TODO: add space between insert if its not there
//                                 //       and also make sure it clips to avoid inserting inside a
//                                 color

//                                 .opt_fixes =
//                                 {{__TOKEN_N::bare_token(__TOKEN_TYPES_N::PUNCTUATION_OPEN_BRACE),
//                                                 self->current().column_number() +
//                                                 self->current().length() + 3}}});

//     if (self->current().token_kind() == __TOKEN_TYPES_N::PUNCTUATION_COLON);
//     auto bad_token = self->current();

//     error::Panic(
//         error::CodeError{.pof      = &bad_token,
//                             .err_code = 0.7008,

//                             // TODO: add space between insert if its not there
//                             //       and also make sure it clips to avoid inserting inside a
//                             color

//                             .opt_fixes =
//                             {{__TOKEN_N::bare_token(__TOKEN_TYPES_N::PUNCTUATION_OPEN_BRACE),
//                                         bad_token.column_number() + bad_token.length() + 3}}});

// }

void parse_namespace(Preprocessor *self) {
    // at the time of call, current() is 'module'
    // module foo::bar {
    // brain process
    // start at module, peek and see if theres a id or : or {
    // if id read the namespace name
    // else if { return (leaving { as current and set self->started_module to true)
    // else if : error saying to add a {
    // else say excepted a {

    QualifiedName namespace_name;
}

void parse_brace(Preprocessor *self) {}

void parse_invocation(Preprocessor *self) {
    // work to do in parse_invocation:
    // keep track of current module scope
    // find a invocation, count number of params and the exact path by walking back
    //      if it cant be found with a associated define error out
    // call the generate location data on the invocation once data is gathered
    // return the iter loc to the start of the invocation (to parse out invocations inside the
    // define)
}

__TOKEN_N::TokenList Preprocessor::parse(std::shared_ptr<preprocessor::ImportNode> parent_node) {
    /* order of parsing (first to last)
        imports - working on now
        defines
        macros
        directives
    */

    for (auto tok_iter : source_tokens) {
        if (!is_source_iter_set()) {
            source_iter = &tok_iter;
        }

        switch (current().token_kind()) {
            case __TOKEN_TYPES_N::KEYWORD_IMPORT:
                parse_import(this);
                break;
            case __TOKEN_TYPES_N::KEYWORD_FFI:
                parse_ffi(this);
                break;
            case __TOKEN_TYPES_N::KEYWORD_DEFINE:
                parse_define(this);
                break;
            case __TOKEN_TYPES_N::KEYWORD_MACRO:
                parse_macro(this);
                break;
            case __TOKEN_TYPES_N::OPERATOR_LOGICAL_NOT:
                parse_invocation(this);
                break;
            case __TOKEN_TYPES_N::KEYWORD_MODULE:
                parse_namespace(this);
                break;
            case __TOKEN_TYPES_N::PUNCTUATION_OPEN_BRACE:
            case __TOKEN_TYPES_N::PUNCTUATION_CLOSE_BRACE:
                parse_brace(this);
                break;
            default:
                break;
        }
    }

    return source_tokens;
}

}  // namespace parser::preprocessor
