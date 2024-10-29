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

#include "controller/include/config/Controller_config.def"
#include "controller/include/shared/file_system.hh"
#include "parser/ast/include/private/base/AST_base.hh"
#include "parser/ast/include/types/AST_jsonify_visitor.hh"
#include "parser/ast/include/types/AST_types.hh"
#include "token/include/private/Token_base.hh"

#define _SILENCE_CXX23_ALIGNED_UNION_DEPRECATION_WARNING
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#ifdef MSVC
#pragma comment(linker, "/STACK:2000000000")  // Set stack size to 2MB
#pragma comment(linker, "/HEAP:2000000000")   // Set heap size to 2MB
#endif

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

// class CXIRCompiler {
//   public:
// #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
//     template <typename T>
//     struct ExecResult {
//         T   output;
//         int return_code{};
//     };
//     template <typename T>
//     [[nodiscard]] ExecResult<T> exec(const std::string &cmd) const;
// #else
//     struct ExecResult {
//         std::string output;
//         int         return_code{};
//     };
//     [[nodiscard]] static ExecResult exec(const std::string &cmd) {
//         std::array<char, 128> buffer;
//         std::string           result;

//         auto *pipe = popen(cmd.c_str(), "r");  // get rid of shared_ptr

//         if (!pipe) {
//             throw std::runtime_error("popen() failed!");
//         }

//         while (feof(pipe) == 0) {
//             if (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
//                 result += buffer.data();
//             }
//         }

//         auto rc = pclose(pipe);

//         if (rc == EXIT_SUCCESS) {
//             return {result, 0};
//         }

//         return {result, 1};
//     }
// #endif

//     void compile_CXIR(generator::CXIR::CXIR &emitter,
//                       const std::string     &out,
//                       bool                   is_debug   = false,
//                       bool                   is_verbose = false) const {
// #if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)
//         compile_CXIR_Windows(emitter, out, is_debug);
// #else
//         compile_CXIR_NonWindows(emitter, out, is_debug, is_verbose);
// #endif
//     }

//   private:

//     void compile_CXIR_NonWindows(generator::CXIR::CXIR &emitter,
//                                  const std::string     &out,
//                                  bool                   is_debug,
//                                  bool                   is_verbose = false) const {
//         std::string cxx = emitter.to_CXIR();

//         std::filesystem::path path        = __CONTROLLER_FS_N::get_cwd();
//         std::filesystem::path source_file = path / "_H1HJA9ZLO_17.helix-compiler.cc";
//         std::ofstream         file(source_file);

//         if (!file) {
//             helix::log<LogLevel::Error>("error creating " + source_file.string() + " file");
//             return;
//         }

//         file << cxx;
//         file.close();

//         std::string compiler        = "c++";
//         auto        compiler_result = exec(compiler + " --version");
//         std::string compile_flags   = "-std=c++23 ";
//         compile_flags += is_debug ? "-g " : "-O2 ";

//         if ((compiler_result.output.find("clang") != std::string::npos) ||
//             (compiler_result.output.find("gcc") != std::string::npos)) {
//             helix::log<LogLevel::Info>(
//                 "using system's '" +
//                 std::string((compiler_result.output.find("clang") != std::string::npos) ? "clang"
//                                                                                         : "gcc") +
//                 "' compiler, with the '" +
//                 ((compiler_result.output.find("clang") != std::string::npos) ? "lld" : "ld") +
//                 "' linker");

//             // -fdiagnostics-format=json << gcc
//             /// -fdiagnostics-show-hotness -fdiagnostics-print-source-range-info  << clang
//             compile_flags += " -fcaret-diagnostics-max-lines=0 -fno-diagnostics-fixit-info "
//                              "-fno-elide-type -fno-diagnostics-show-line-numbers "
//                              "-fno-color-diagnostics -fno-diagnostics-show-option ";

//         } else {
//             helix::log<LogLevel::Error>("aborting. unknown compiler: " + compiler_result.output);
//             return;
//         }

//         compile_flags += "-fno-omit-frame-pointer -Wl,-w,-rpath,/usr/local/lib ";
//         compile_flags += "\"" + source_file.string() + "\" -o \"" + (path / out).string() + "\"";

//         auto compile_result = exec("c++ " + compile_flags + " 2>&1");
//         if (compile_result.return_code != 0) {
//             std::vector<std::string> lines;
//             std::istringstream       stream(compile_result.output);

//             for (std::string line; std::getline(stream, line);) {
//                 if (line.starts_with('/')) {
//                     lines.push_back(line);
//                 }
//             }

//             for (auto &line : lines) {
//                 auto err = parse_clang_err(line);

//                 if (!std::filesystem::exists(std::get<2>(err))) {
//                     error::Panic(error::CompilerError{
//                         .err_code     = 0.8245,
//                         .err_fmt_args = {"error at: " + std::get<2>(err) + std::get<1>(err)},
//                     });

//                     continue;
//                 }

//                 std::pair<size_t, size_t> err_t = {std::get<1>(err).find_first_not_of(' '),
//                                                    std::get<1>(err).find(':') -
//                                                        std::get<1>(err).find_first_not_of(' ')};

//                 error::Level level = std::map<string, error::Level>{
//                     {"error", error::Level::ERR},                       //
//                     {"warning", error::Level::WARN},                    //
//                     {"note", error::Level::NOTE}                        //
//                 }[std::get<1>(err).substr(err_t.first, err_t.second)];  //

//                 std::string msg = std::get<1>(err).substr(err_t.first + err_t.second + 1);

//                 msg = msg.substr(msg.find_first_not_of(' '));

//                 error::Panic(error::CodeError{
//                     .pof          = &std::get<0>(err),
//                     .err_code     = 0.8245,
//                     .err_fmt_args = {msg},
//                     .mark_pof     = false,
//                     .level        = level,
//                     .indent       = static_cast<size_t>((level == error::NOTE) ? 1 : 0),
//                 });
//             }

//             if (is_verbose || !error::HAS_ERRORED) {
//                 helix::log<LogLevel::Info>("compilation passed");
//                 helix::log<LogLevel::Error>((is_verbose ? "compiler output:\n" : "linker failed. ") +
//                                      compile_result.output);
//             } else {
//                 helix::log<LogLevel::Error>("compilation failed");
//             }

//             helix::log<LogLevel::Error>("aborting...");
//             goto cleanup;
//         }

//         helix::log<LogLevel::Info>("lowered " +
//                             (emitter.get_file_name().has_value() ? emitter.get_file_name().value()
//                                                                  : source_file.string()) +
//                             " and compiled cxir");
//         helix::log<LogLevel::Info>("compiled successfully to " + (path / out).string());

//     cleanup:
//         std::filesystem::remove(source_file);
//     }
// };

int main(int argc, char **argv) {
    auto compiler = CompilationUnit();
    int  result   = 1;

    try {
        result = compiler.compile(argc, argv);
    } catch (error::Panic &) {  // hard error
        return 69;              // nice
    }

    if (LSP_MODE && error::HAS_ERRORED) {
        std::vector<neo::json> errors;

        for (const auto &err : error::ERRORS) {
            errors.push_back(err.to_json());
        }

        neo::json error_json;
        error_json.add("errors", errors);

        print(error_json);

        return 1;  // soft error
    }

    return result;
}

/*
using the new toolchain the entire process can happen like this:

template <std::size_t N>
int helix_compiler(std::array<std::string, N> args) {
    InvocationManager       invoke      = InvocationManager(args, true);
    CompilationUnit         compile     = CompilationUnit(invoke);
    PrettyDiagnosticHandler diag_handle = PrettyDiagnosticHandler();

    // frontend processors
    compile.add_frontend_processor(LexicalProcessor());
    compile.add_frontend_processor(PreProcessor());
    compile.add_frontend_processor(ASTProcessor());

    // backend processors
    compile.add_backend_processor(CXIRGenerator());
    compile.add_backend_processor(ExecutableGenerator<OutputFormat::Executable>());

    // set diagnostics
    compile.set_diagnostic_handler(diag_handle);

    if (compile.execute() != InvocationManager::Success) {
        switch (compile.exit_state) {
            case InvocationManager::CompilerError:
                log(
                    "compiler internal error: ",
                    diag_handle
                        .get<InvocationManager::CompilerError>()
                        .format()
                        .to_string()
                ); break;

            case InvocationManager::CodeError:
                log(
                    "user code error: ",
                    diag_handle
                        .get<InvocationManager::CodeError>()
                        .format()
                        .to_string()
                ); break;

            case InvocationManager::UnrecoverableError:
                log(
                    "fatal error: ",
                    diag_handle
                        .get<InvocationManager::UnrecoverableError>()
                        .format()
                        .to_string()
                ); break;

            case InvocationManager::SystemError:
                log(
                    "sys call error: ",
                    diag_handle
                        .get<InvocationManager::SystemError>()
                        .format()
                        .to_string()
                ); break;
        };

        diag_handle.panic(PrettyDiagnosticHandler::ColorMode::ON);
    }

    return 0;
}

*/

