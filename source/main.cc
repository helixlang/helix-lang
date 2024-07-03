/**
 * @author Dhruvan Kartik
 * @copyright Copyright (c) 2024 (CC BY 4.0)
 *
 * @note This code is part of the Helix Language Project and is licensed under the Attribution 4.0
 * International license (CC BY 4.0). You are allowed to use, modify, redistribute, and create
 * derivative works, even for commercial purposes, provided that you give appropriate credit,
 * provide a link to the license, and indicate if changes were made. For more information, please
 * visit: https://creativecommons.org/licenses/by/4.0/ SPDX-License-Identifier: CC-BY-4.0
 *
 * @note This code is provided by the creators of Helix. Visit our website at:
 * https://helix-lang.com/ for more information.
 */

#include <chrono>
#include <include/printV2>
#include <memory>

#include "token/include/lexer.hh"
#include "token/include/token.hh"
#include "parser/ast/include/nodes.hh"
#include "cli/include/cli.hh"
#include "parser/ast/include/ast.hh"
#include "tools/controllers/include/file_system.hh"
#include "parser/preprocessor/include/preprocessor.hh"

int main(int argc, char **argv) {
    using namespace token;
    using namespace parser;
    using namespace lexer;

    auto start = std::chrono::high_resolution_clock::now();



    std::string file_name = "/Volumes/Container/Projects/Helix/helix-lang/tests/main.hlx"; // relative to current working dir in POSIX shell (cmd/bash)

    // read the file and tokenize its contents : stage 0
    TokenList tokens = Lexer(file_system::read_file(file_name), file_name).tokenize();

    // preprocess the tokens : stage 1
    Preprocessor(tokens, "main").parse();

    // end the timer and calculate the duration
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    // preprocessor::import_tree->print_tree(preprocessor::import_tree->get_root());

    // print the preprocessed tokens
    // print_tokens(tokens);


    auto node = std::make_unique<ast::Parentheses<ast::StringLiteral>>(tokens);
    
    node->parse();

    print(node->to_string());
    
    // Print the time taken in nanoseconds and milliseconds
    print("time taken: ", diff.count() * 1e+9, " ns");
    print("            ", diff.count() * 1000, " ms");

    return 0;
}
