#include "controller/include/config/Controller_config.def"
#include "controller/include/shared/file_system.hh"
#include "parser/ast/include/private/base/AST_base.hh"
#include "parser/ast/include/types/AST_jsonify_visitor.hh"
#include "parser/ast/include/types/AST_types.hh"
#include "token/include/private/Token_base.hh"

#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <neo-panic/include/error.hh>
#include <neo-pprint/include/hxpprint.hh>
#include <string>
#include <vector>

#include "controller/include/Controller.hh"
#include "generator/include/CX-IR/CXIR.hh"
#include "lexer/include/lexer.hh"
#include "controller/include/tooling/tooling.hh"
#include "controller/include/shared/logger.hh"

extern bool LSP_MODE;

int CompilationUnit::compile(int argc, char **argv) {
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    parser::ast::NodeT<parser::ast::node::Program>              ast;
    std::filesystem::path                                       in_file_path;
    generator::CXIR::CXIR                                       emitter;
    parser::lexer::Lexer                                        lexer;
    __TOKEN_N::TokenList                                        tokens;

    __CONTROLLER_CLI_N::CLIArgs parsed_args(argc, argv, "0.0.1-alpha-2012");
    check_exit(parsed_args);

    if (parsed_args.quiet || parsed_args.lsp_mode) {
        NO_LOGS           = true;
        error::SHOW_ERROR = false;
    }

    if (parsed_args.verbose) {
        helix::log<LogLevel::Debug>(parsed_args.get_all_flags);
    }

    start        = std::chrono::high_resolution_clock::now();
    in_file_path = __CONTROLLER_FS_N::normalize_path(parsed_args.file);
    lexer        = {__CONTROLLER_FS_N::read_file(in_file_path.generic_string()), in_file_path.generic_string()};
    tokens       = lexer.tokenize();

    helix::log<LogLevel::Info>("tokenized");

    // preprocessor - missing for now
    helix::log<LogLevel::Info>("preprocessed");

    if (parsed_args.emit_tokens) {
        helix::log<LogLevel::Debug>(tokens.to_json());
        print_tokens(tokens);
    }

    remove_comments(tokens);
    ast = parser::ast::make_node<parser::ast::node::Program>(tokens);

    if (!ast) {
        helix::log<LogLevel::Error>("aborting...");
        return 1;
    }

    ast->parse();
    helix::log<LogLevel::Info>("parsed");

    if (parsed_args.emit_ast) {
        parser::ast::visitor::Jsonify json_visitor;
        ast->accept(json_visitor);

        if (parsed_args.lsp_mode) {
            print(json_visitor.json.to_string());
            return 0;
        }

        helix::log<LogLevel::Debug>(json_visitor.json.to_string());
    }

    if (error::HAS_ERRORED) {
        LSP_MODE = parsed_args.lsp_mode;
        return 0;
    }

    ast->accept(emitter);
    helix::log<LogLevel::Info>("emitted cx-ir");

    if (parsed_args.emit_ir) {
        emit_cxir(emitter, parsed_args.verbose);
    }

    std::filesystem::path out_file = determine_output_file(parsed_args, in_file_path);
    // helix::log<LogLevel::Info>("output file: " + out_file.generic_string());

    if (error::HAS_ERRORED) {
        helix::log<LogLevel::Error>("aborting... due to previous errors");
        return 1;
    }

    flag::CompileFlags action_flags;

    if (parsed_args.build_mode == __CONTROLLER_CLI_N::CLIArgs::MODE::DEBUG_) {
        action_flags |= flag::CompileFlags(flag::types::CompileFlags::Debug);
    }

    if (parsed_args.verbose) {
        action_flags |= flag::CompileFlags(flag::types::CompileFlags::Verbose);
    }

    compiler.compile_CXIR(CXXCompileAction::init(emitter, out_file, action_flags, parsed_args.cxx_args));

    if (error::HAS_ERRORED || parsed_args.lsp_mode) {
        LSP_MODE = parsed_args.lsp_mode;
        return 0;
    }

    log_time(start, parsed_args.verbose, std::chrono::high_resolution_clock::now());
    return 0;
}

void CompilationUnit::remove_comments(__TOKEN_N::TokenList &tokens) {
    __TOKEN_N::TokenList new_tokens;

    for (auto &token : tokens) {
        if (token->token_kind() != __TOKEN_N::PUNCTUATION_SINGLE_LINE_COMMENT &&
            token->token_kind() != __TOKEN_N::PUNCTUATION_MULTI_LINE_COMMENT) {
            new_tokens.push_back(token.current().get());
        }
    }

    tokens = new_tokens;
}

void CompilationUnit::emit_cxir(const generator::CXIR::CXIR &emitter, bool verbose) {
    helix::log<LogLevel::Info>("emitting cx-ir...");

    if (verbose) {
        helix::log<LogLevel::Debug>("\n", colors::fg16::yellow, emitter.to_CXIR(), colors::reset);
    } else {
        helix::log<LogLevel::Info>(
            "\n", colors::fg16::yellow, emitter.to_readable_CXIR(), colors::reset);
    }
}

std::filesystem::path CompilationUnit::determine_output_file(const __CONTROLLER_CLI_N::CLIArgs &parsed_args,
                                            const std::filesystem::path       &in_file_path) {

    std::string out_file = (parsed_args.output_file.has_value())
                                ? parsed_args.output_file.value()
                                : std::filesystem::path(in_file_path).stem().generic_string();

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
    out_file += ".exe";
#endif

    return __CONTROLLER_FS_N::normalize_path_no_check(out_file);
}

void CompilationUnit::log_time(const std::chrono::high_resolution_clock::time_point &start,
                        bool                                                  verbose,
                        const std::chrono::high_resolution_clock::time_point &end) {

    std::chrono::duration<double> diff = end - start;

    if (verbose) {
        helix::log<LogLevel::Debug>("time taken: " + std::to_string(diff.count() * 1e+9) + " ns");
        helix::log<LogLevel::Debug>("            " + std::to_string(diff.count() * 1000) + " ms");
    }
}