#include <type_traits>

#include "controller/include/config/cxx_flags.hh"
#include "controller/include/shared/eflags.hh"
#include "controller/include/shared/logger.hh"
#include "controller/include/tooling/tooling.hh"

template <typename... Flags>
std::string make_command(const flag::types::Compiler _Compiler, Flags... flags) {
    std::string compile_cmd;

    // for each flag call flag->clang after checking if its of type cxx::flags::CX
    (void)std::initializer_list<int>{(
        [&compile_cmd, &_Compiler](auto flag) {
            if constexpr (std::is_same_v<decltype(flag), cxx::flags::CF>) {
                if (_Compiler == flag::types::Compiler::Clang) {
                    compile_cmd += std::string((flag).clang) + " ";
                } else if (_Compiler == flag::types::Compiler::GCC) {
                    compile_cmd += std::string((flag).gcc) + " ";
                } else if (_Compiler == flag::types::Compiler::MSVC) {
                    compile_cmd += std::string((flag).msvc) + " ";
                } else if (_Compiler == flag::types::Compiler::MingW) {
                    compile_cmd += std::string((flag).mingw) + " ";
                } else {
                    throw std::runtime_error("unknown compiler");
                }
            } else if constexpr (std::is_same_v<decltype(flag), std::string> ||
                                 std::is_same_v<decltype(flag), const char *>) {
                compile_cmd += flag + std::string(" ");
            } else {
                static_assert(false, "invalid flag type");
            }
        }(flags),
        0)...};

    return compile_cmd;
}

CXIRCompiler::CompileResult CXIRCompiler::CXIR_CXX(CXXCompileAction action) const {
    /// identify the compiler
    ExecResult            compile_result = exec(action.cxx_compiler + " --version");
    flag::types::Compiler compiler       = flag::types::Compiler::Custom;
    bool is_verbose                      = action.flags.contains(EFlags(flag::types::CompileFlags::Verbose));

    if (compile_result.return_code != 0) {
        helix::log<LogLevel::Error>("failed to identify the compiler");
        return {compile_result, flag::ErrorType(flag::types::ErrorType::NotFound)};
    }

    if (compile_result.output.find("clang") != std::string::npos) {
        compiler = flag::types::Compiler::Clang;
    } else if (compile_result.output.find("gcc") != std::string::npos) {
        compiler = flag::types::Compiler::GCC;
    } else if (compile_result.output.find("msvc") != std::string::npos) {
        compiler = flag::types::Compiler::MSVC;
    } else if (compile_result.output.find("mingw") != std::string::npos) {
        compiler = flag::types::Compiler::MingW;
    }

    std::string compile_cmd = action.cxx_compiler + " ";
    
    /// start with flags we know are going to be present
    compile_cmd += make_command(  // ...
        compiler,
        ((action.flags.contains(flag::types::CompileFlags::Debug))
             ? cxx::flags::debugModeFlag
             : cxx::flags::optimizationLevel3),

        cxx::flags::cxxStandardFlag,
        cxx::flags::stdCXX23Flag,
        cxx::flags::enableExceptionsFlag,

        // cxx::flags::noDefaultLibrariesFlag,
        // cxx::flags::noCXXSTDLibrariesFlag,
        // cxx::flags::noCXXSTDIncludesFlag,
        // cxx::flags::noBuiltinIncludesFlag,
        // FIXME: add these later

        cxx::flags::noOmitFramePointerFlag,
        cxx::flags::noColorDiagnosticsFlag,
        cxx::flags::noDiagnosticsFixitFlag,
        cxx::flags::noDiagnosticsShowLineNumbersFlag,
        cxx::flags::noDiagnosticsShowOptionFlag,
        cxx::flags::caretDiagnosticsMaxLinesFlag,
        cxx::flags::noElideTypeFlag,

#if IS_UNIX
        "-Wl,-w,-rpath,/usr/local/lib",
#endif
        cxx::flags::warnAllFlag,
        cxx::flags::outputFlag,
        action.cc_output.generic_string()  // output
    );

    /// add any additional flags passed into the action
    for (auto &flag : action.cxx_args) {
        compile_cmd += flag + " ";
    }

    /// add the source file in a normalized path format
    compile_cmd += "\"" + action.cc_source.generic_string() + "\"";

    /// redirect stderr to stdout
    compile_cmd += " 2>&1";
    
    /// execute the command
    compile_result = exec(compile_cmd);
    
    if (is_verbose) {
        helix::log<LogLevel::Debug>("compile command: " + compile_cmd);
        helix::log<LogLevel::Debug>("compiler output:\n" + compile_result.output);
    }

    /// parse the output and show errors after translating them to helix errors
    std::vector<std::string> lines;
    std::istringstream       stream(compile_result.output);

    for (std::string line; std::getline(stream, line);) {
        if (line.starts_with('/')) {
            lines.push_back(line);
        }
    }

    for (auto &line : lines) {
        ErrorPOFNormalized err;
    
        if (compiler == flag::types::Compiler::Clang) {
            err = CXIRCompiler::parse_clang_err(line);
        } else if (compiler == flag::types::Compiler::GCC) {
            err = CXIRCompiler::parse_gcc_err(line);
        } else if (compiler == flag::types::Compiler::MSVC) {
            err = CXIRCompiler::parse_msvc_err(line);
        } else {
            helix::log<LogLevel::Error>("unknown c++ compiler, raw output shown");
            helix::log<LogLevel::Info>("output ------------>");
            helix::log<LogLevel::Info>(compile_result.output);
            helix::log<LogLevel::Info>("<------------ output");

            if (compile_result.return_code == 0) {
                helix::log<LogLevel::Info>("lowered " + action.helix_src.generic_string() + " and compiled cxir");
                helix::log<LogLevel::Info>("compiled successfully to " + action.cc_output.generic_string());
                return {compile_result, flag::ErrorType(flag::types::ErrorType::Success)};
            }
            
            return {compile_result, flag::ErrorType(flag::types::ErrorType::Error)};
           
        }

        if (!std::filesystem::exists(std::get<2>(err))) {
            error::Panic _(error::CompilerError{
                .err_code     = 0.8245,
                .err_fmt_args = {"error at: " + std::get<2>(err) + std::get<1>(err)},
            });

            continue;
        }

        std::pair<size_t, size_t> err_t = {std::get<1>(err).find_first_not_of(' '),
                                           std::get<1>(err).find(':') -
                                               std::get<1>(err).find_first_not_of(' ')};

        error::Level level = std::map<string, error::Level>{
            {"error", error::Level::ERR},                       //
            {"warning", error::Level::WARN},                    //
            {"note", error::Level::NOTE}                        //
        }[std::get<1>(err).substr(err_t.first, err_t.second)];  //

        std::string msg = std::get<1>(err).substr(err_t.first + err_t.second + 1);

        msg = msg.substr(msg.find_first_not_of(' '));

        error::Panic(error::CodeError{
            .pof          = &std::get<0>(err),
            .err_code     = 0.8245,
            .err_fmt_args = {msg},
            .mark_pof     = false,
            .level        = level,
            .indent       = static_cast<size_t>((level == error::NOTE) ? 1 : 0),
        });
    }

    if (compile_result.return_code == 0) {
        helix::log<LogLevel::Info>("lowered " + action.helix_src.generic_string() + " and compiled cxir");
        helix::log<LogLevel::Info>("compiled successfully to " + action.cc_output.generic_string());
        return {compile_result, flag::ErrorType(flag::types::ErrorType::Success)};
    }
    
    return {compile_result,
            flag::ErrorType(error::HAS_ERRORED ? flag::types::ErrorType::Error
                                               : flag::types::ErrorType::Success)};
}

//     std::string compiler        = "c++";
//     auto        compiler_result = exec(compiler + " --version");
//     std::string compile_flags   = "-std=c++23 ";
//     compile_flags += is_debug ? "-g " : "-O2 ";

//     if ((compiler_result.output.find("clang") != std::string::npos) ||
//         (compiler_result.output.find("gcc") != std::string::npos)) {
//         helix::log<LogLevel::Info>(
//             "using system's '" +
//             std::string((compiler_result.output.find("clang") != std::string::npos) ? "clang"
//                                                                                     : "gcc")
//                                                                                     +
//             "' compiler, with the '" +
//             ((compiler_result.output.find("clang") != std::string::npos) ? "lld" : "ld") +
//             "' linker");

//         // -fdiagnostics-format=json << gcc
//         /// -fdiagnostics-show-hotness -fdiagnostics-print-source-range-info  << clang
//         compile_flags += " -fcaret-diagnostics-max-lines=0 -fno-diagnostics-fixit-info "
//                             "-fno-elide-type -fno-diagnostics-show-line-numbers "
//                             "-fno-color-diagnostics -fno-diagnostics-show-option ";

//     } else {
//         helix::log<LogLevel::Error>("aborting. unknown compiler: " + compiler_result.output);
//         return;
//     }

//     compile_flags += "-fno-omit-frame-pointer -Wl,-w,-rpath,/usr/local/lib ";
//     compile_flags += "\"" + source_file.generic_string() + "\" -o \"" + (path / out).generic_string() + "\"";

//     auto compile_result = exec("c++ " + compile_flags + " 2>&1");
//     if (compile_result.return_code != 0) {
//         std::vector<std::string> lines;
//         std::istringstream       stream(compile_result.output);

//         for (std::string line; std::getline(stream, line);) {
//             if (line.starts_with('/')) {
//                 lines.push_back(line);
//             }
//         }

//         for (auto &line : lines) {
//             auto err = parse_clang_err(line);

//             if (!std::filesystem::exists(std::get<2>(err))) {
//                 error::Panic(error::CompilerError{
//                     .err_code     = 0.8245,
//                     .err_fmt_args = {"error at: " + std::get<2>(err) + std::get<1>(err)},
//                 });

//                 continue;
//             }

//             std::pair<size_t, size_t> err_t = {std::get<1>(err).find_first_not_of(' '),
//                                                 std::get<1>(err).find(':') -
//                                                     std::get<1>(err).find_first_not_of(' ')};

//             error::Level level = std::map<string, error::Level>{
//                 {"error", error::Level::ERR},                       //
//                 {"warning", error::Level::WARN},                    //
//                 {"note", error::Level::NOTE}                        //
//             }[std::get<1>(err).substr(err_t.first, err_t.second)];  //

//             std::string msg = std::get<1>(err).substr(err_t.first + err_t.second + 1);

//             msg = msg.substr(msg.find_first_not_of(' '));

//             error::Panic(error::CodeError{
//                 .pof          = &std::get<0>(err),
//                 .err_code     = 0.8245,
//                 .err_fmt_args = {msg},
//                 .mark_pof     = false,
//                 .level        = level,
//                 .indent       = static_cast<size_t>((level == error::NOTE) ? 1 : 0),
//             });
//         }

//         if (is_verbose || !error::HAS_ERRORED) {
//             helix::log<LogLevel::Info>("compilation passed");
//             helix::log<LogLevel::Error>((is_verbose ? "compiler output:\n" : "linker failed.
//             ") +
//                                     compile_result.output);
//         } else {
//             helix::log<LogLevel::Error>("compilation failed");
//         }

//         helix::log<LogLevel::Error>("aborting...");
//         goto cleanup;
//     }

//     helix::log<LogLevel::Info>("lowered " +
//                         (emitter.get_file_name().has_value() ?
//                         emitter.get_file_name().value()
//                                                                 : source_file.generic_string()) +
//                         " and compiled cxir");
//     helix::log<LogLevel::Info>("compiled successfully to " + (path / out).generic_string());

// cleanup:
//     std::filesystem::remove(source_file);
