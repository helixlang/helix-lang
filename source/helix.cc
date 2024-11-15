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

#define _SILENCE_CXX23_ALIGNED_UNION_DEPRECATION_WARNING
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include <neo-panic/include/error.hh>
#include <neo-pprint/include/hxpprint.hh>
#include <vector>

#include "controller/include/tooling/tooling.hh"

int main(int argc, char **argv) {
    std::vector<neo::json> errors;
    auto compiler = CompilationUnit();
    int  result   = 1;

    try {
        result = compiler.compile(argc, argv);
    } catch (error::Panic &e) {  // hard error
        errors.push_back(e.final_err.to_json());
    }

    if (LSP_MODE && error::ERRORS.size() > 0) {
        for (const auto &err : error::ERRORS) {
            if (err.line == 0 && err.col == 0) {
                continue;
            }
            
            errors.push_back(err.to_json());
        }

        neo::json error_json("error");
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
