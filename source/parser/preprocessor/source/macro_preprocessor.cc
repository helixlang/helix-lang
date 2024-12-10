#include <cstddef>
#include <filesystem>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "parser/ast/include/AST.hh"
#include "token/include/Token.hh"
#include "token/include/config/Token_config.def"
#include "token/include/private/Token_generate.hh"
#include "token/include/private/Token_list.hh"

/*
macro_def := 'macro' IDENTIFIER ( '(' IDENTIFIER ( ',' IDENTIFIER )* ')' )? '{' ANY* '}' ';'

FIXME  : macro rules:
#...  -> stringify
\...  -> concat
\#... -> un-stringify
*/

struct MacroDef {
    std::vector<__TOKEN_N::Token> params;
    __TOKEN_N::TokenList          body;
    __TOKEN_N::Token              name;
    __TOKEN_N::TokenList          location;
};

std::map<std::filesystem::path, std::vector<MacroDef>> macros;

class MacroProcessor {
  private:
    __TOKEN_N::TokenList &tokens;  // NOLINT - this is a reference intentionally
                                   // (i cba to make it a pointer lol)

  public:
    bool parse_invoke() {
        /*
            module bar {
                macro foo!: abc;
            }

            module baz {
                let abc = 10;
            }

            bar::foo!; // expands to: abc
            baz::bar::foo!; // error since foo! isn't defined in baz unless baz uses the module bar
            baz::(bar::foo!); // alternate
        */

        return false;
    }

    void parse() {
        __TOKEN_N::TokenList::TokenListIter iter = tokens.begin();
        std::filesystem::path               current_file;

        size_t start_pos = std::numeric_limits<size_t>::max();

        size_t                                                 level = 0;
        std::vector<std::pair<size_t, __TOKEN_N::TokenList>> module_level_stack;

        MacroDef macro;

        while (iter.remaining_n() > 0) {
            if (current_file != iter.current().get().file_name()) {
                current_file = iter.current().get().file_name();
            }

            switch (iter.current().get().token_kind()) {
                case __TOKEN_N::PUNCTUATION_OPEN_BRACE:
                    level += 1;
                    break;

                case __TOKEN_N::PUNCTUATION_CLOSE_BRACE:
                    if (level == module_level_stack.back().first) {
                        module_level_stack.pop_back();
                    }
                    if (!iter.peek().has_value()) {
                        /// error
                        break;
                    }
                    if (iter.peek().value().get().token_kind() !=
                        __TOKEN_N::PUNCTUATION_SEMICOLON) {
                        // error
                    }

                    macros[current_file].push_back(macro);
                    break;
                case __TOKEN_N::KEYWORD_MODULE: {
                    // figure out the module name: goo::bam::boo
                    size_t start = iter.position();

                    bool found = false;

                    while (iter.remaining_n() > 0 && !found) {
                        // at this state a first find macros should be done if an invoke happens
                        // before any macros are found error.
                        switch (iter.current().get().token_kind()) {
                            case __TOKEN_N::PUNCTUATION_OPEN_BRACE:
                                found = true;
                                break;
                            case __TOKEN_N::IDENTIFIER:
                            case __TOKEN_N::OPERATOR_SCOPE:
                            default:
                                iter.advance();
                                break;
                        }
                    }

                    module_level_stack.emplace_back(level, tokens.slice(start, static_cast<size_t>(iter.position())));
                    break;

                } case __TOKEN_N::KEYWORD_MACRO:
                    /// parse macro ...! ( '(' ... (',' ...)* ')') {};
                    /// or    macro ...!: ...;
                    /// parse for invokes after each macro gets found

                    
                    
                default:
                    iter.advance();
                    break;
            }
        }
    }
};

/*

macro namespace!: FOO::BAR::BUZZ;

module namespace! {
    macro mercatro!: 128;
}

namespace!::mercatro! == 128; // true

*/