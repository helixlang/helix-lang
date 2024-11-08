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

#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <neo-panic/include/error.hh>
#include <neo-pprint/include/hxpprint.hh>
#include <stdexcept>
#include <string>
#include <vector>

#include "controller/include/Controller.hh"
#include "controller/include/config/Controller_config.def"
#include "controller/include/shared/file_system.hh"
#include "controller/include/shared/logger.hh"
#include "controller/include/tooling/tooling.hh"
#include "generator/include/CX-IR/CXIR.hh"
#include "lexer/include/lexer.hh"
#include "parser/ast/include/private/base/AST_base.hh"
#include "parser/ast/include/types/AST_jsonify_visitor.hh"
#include "parser/ast/include/types/AST_types.hh"
#include "token/include/private/Token_base.hh"

extern bool LSP_MODE;

template <typename T>
void process_paths(std::vector<T>                     &paths,
                   std::vector<std::filesystem::path> &add_to,
                   std::filesystem::path &
                       base_to,  //< must be a file not a dir, it is checked also must be normalized
                   std::vector<std::filesystem::path> add_after_base,
                   bool                               is_dir = true) {
    std::vector<std::filesystem::path> normalized_input;
    std::filesystem::path              cwd = __CONTROLLER_FS_N::get_cwd();
    std::filesystem::path              exe = __CONTROLLER_FS_N::get_exe();

    if constexpr (!std::is_same_v<T, std::string> && !std::is_same_v<T, std::filesystem::path>) {
        throw std::runtime_error("invalid usage of process_paths only string or path vec allowed");
    }

    if (!paths.empty()) {
        for (auto &dir : paths) {
            normalized_input.emplace_back(dir);
        }
    }

    /// first path added is always the base_to if its not the cwd, then the cwd then the rest
    if (std::filesystem::is_regular_file(base_to)) {
        if (!std::filesystem::exists(base_to.parent_path()) &&
            !std::filesystem::is_directory(base_to.parent_path())) {
            helix::log<LogLevel::Warning>(
                "specified include dir is not a directory or does not exist: \'" +
                base_to.parent_path().generic_string() + "\'");
        } else if (base_to.parent_path() != cwd) {
            add_to.emplace_back(base_to.parent_path());
        }

    } else {
        throw std::runtime_error("input must be a file.");
    }

    // add the cwd
    add_to.emplace_back(cwd);

    if (!add_after_base.empty()) {
        for (auto &dir : add_after_base) {
            if (std::filesystem::is_directory(dir)) {
                add_to.emplace_back(dir);
            } else {
                helix::log<LogLevel::Warning>(
                    "specified include dir is not a directory or does not exist: \'" +
                    dir.generic_string() + "\'");
            }
        }
    }

    // add the remaining paths
    if (!normalized_input.empty()) {
        for (const auto &dir : normalized_input) {
            auto path = __CONTROLLER_FS_N::resolve_path(dir, false);

            if (path.has_value() && !std::filesystem::is_directory(path.value())) {
                helix::log<LogLevel::Warning>(
                    "specified include dir is not a directory or does not exist: \'" +
                    dir.generic_string() + "\'");

                continue;
            }

            add_to.emplace_back(path.value());
        }
    }
}

int CompilationUnit::compile(int argc, char **argv) {
    __CONTROLLER_CLI_N::CLIArgs parsed_args(argc, argv, "0.0.1-alpha-2012");
    check_exit(parsed_args);

    return compile(parsed_args);
}

/// compile and return the path of the compiled file without calling the linker
/// ret codes: 0 - success, 1 - error, 2 - lsp mode
std::pair<CXXCompileAction, int>
CompilationUnit::compile_wet(__CONTROLLER_CLI_N::CLIArgs &parsed_args) {
    parser::ast::NodeT<parser::ast::node::Program> ast;
    std::vector<std::filesystem::path>             import_dirs;
    std::vector<std::filesystem::path>             link_dirs;
    std::filesystem::path                          in_file_path;
    generator::CXIR::CXIR                          emitter(true);
    parser::lexer::Lexer                           lexer;
    __TOKEN_N::TokenList                           tokens;

    if (parsed_args.quiet || parsed_args.lsp_mode) {
        NO_LOGS           = true;
        error::SHOW_ERROR = false;
    }

    if (parsed_args.verbose) {
        helix::log<LogLevel::Debug>(parsed_args.get_all_flags);
    }

    in_file_path = __CONTROLLER_FS_N::normalize_path(parsed_args.file);
    lexer        = {__CONTROLLER_FS_N::read_file(in_file_path.generic_string()),
                    in_file_path.generic_string()};
    tokens       = lexer.tokenize();

    helix::log<LogLevel::Info>("tokenized");

    process_paths(parsed_args.library_dirs,
                  link_dirs,
                  in_file_path,
                  {__CONTROLLER_FS_N::get_exe().parent_path().parent_path() / "libs"});

    process_paths(parsed_args.include_dirs,
                  import_dirs,
                  in_file_path,
                  {__CONTROLLER_FS_N::get_exe().parent_path().parent_path() / "pkgs"});

    if (parsed_args.verbose) {
        helix::log<LogLevel::Debug>("import dirs: [");
        for (const auto &dir : import_dirs) {
            helix::log<LogLevel::Debug>(dir.generic_string() + ", ");
        }
        helix::log<LogLevel::Debug>("]");

        helix::log<LogLevel::Debug>("link dirs: [");
        for (const auto &dir : link_dirs) {
            helix::log<LogLevel::Debug>(dir.generic_string() + ", ");
        }
        helix::log<LogLevel::Debug>("]");
    }

    // ImportProcessor(tokens, import_dirs)
    helix::log<LogLevel::Info>("preprocessed");

    if (parsed_args.emit_tokens) {
        helix::log<LogLevel::Debug>(tokens.to_json());
        print_tokens(tokens);
    }

    remove_comments(tokens);
    ast = parser::ast::make_node<parser::ast::node::Program>(tokens);

    if (!ast) {
        helix::log<LogLevel::Error>("aborting...");
        return {{}, 1};
    }

    ast->parse();
    helix::log<LogLevel::Info>("parsed");

    if (parsed_args.emit_ast) {
        parser::ast::visitor::Jsonify json_visitor;
        ast->accept(json_visitor);

        if (parsed_args.lsp_mode) {
            print(json_visitor.json.to_string());
            return {{}, 2};
        }

        helix::log<LogLevel::Debug>(json_visitor.json.to_string());
    }

    if (error::HAS_ERRORED) {
        LSP_MODE = parsed_args.lsp_mode;
        return {{}, 2};
    }

    ast->accept(emitter);
    helix::log<LogLevel::Info>("emitted cx-ir");

    if (parsed_args.emit_ir) {
        emit_cxir(emitter, parsed_args.verbose);
    }

    std::filesystem::path out_file = determine_output_file(parsed_args, in_file_path);

    if (error::HAS_ERRORED) {
        helix::log<LogLevel::Error>("aborting... due to previous errors");
        return {{}, 1};
    }

    flag::CompileFlags action_flags;

    if (parsed_args.build_mode == __CONTROLLER_CLI_N::CLIArgs::MODE::DEBUG_) {
        action_flags |= flag::CompileFlags(flag::types::CompileFlags::Debug);
    }

    if (parsed_args.verbose) {
        action_flags |= flag::CompileFlags(flag::types::CompileFlags::Verbose);
    }

    return {CXXCompileAction::init(emitter, out_file, action_flags, parsed_args.cxx_args), 0};
}

int CompilationUnit::compile(__CONTROLLER_CLI_N::CLIArgs &parsed_args) {
    std::chrono::time_point<std::chrono::high_resolution_clock> start =
        std::chrono::high_resolution_clock::now();
    auto [action, result] = compile_wet(parsed_args);

    switch (result) {
        case 0:
            break;

        case 1:
            return 1;

        case 2:
            return 0;
    }

    compiler.compile_CXIR(std::move(action));

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

std::filesystem::path
CompilationUnit::determine_output_file(const __CONTROLLER_CLI_N::CLIArgs &parsed_args,
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