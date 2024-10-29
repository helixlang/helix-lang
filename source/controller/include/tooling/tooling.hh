#ifndef __TOOLING_H__
#define __TOOLING_H__

#include <chrono>
#include <filesystem>
#include <neo-panic/include/error.hh>
#include <neo-pprint/include/hxpprint.hh>
#include <numeric>
#include <random>
#include <string>
#include <utility>

#include "controller/include/Controller.hh"
#include "controller/include/config/Controller_config.def"
#include "controller/include/shared/eflags.hh"
#include "controller/include/shared/logger.hh"
#include "generator/include/CX-IR/CXIR.hh"
#include "token/include/private/Token_base.hh"

#define IS_UNIX                                                                                    \
    (defined(__unix__) || defined(__APPLE__) || defined(__linux__) || defined(__FreeBSD__) ||      \
     defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__DragonFly__) || \
     defined(__MACH__))

#define IS_WIN32 (defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64))

namespace flag {
namespace types {
    enum class CompileFlags : u8 {
        None    = 0,
        Debug   = 1 << 0,
        Verbose = 1 << 1,
    };

    enum class Compiler : u8 {
        MSVC,
        GCC,
        Clang,
        MingW,
        Custom,
    };

    enum class ErrorType : u8 { NotFound, Error, Success };
}  // namespace types

using CompileFlags = EFlags<flag::types::CompileFlags>;
using Compiler     = EFlags<flag::types::Compiler>;
using ErrorType    = EFlags<flag::types::ErrorType>;
}  // namespace flag

inline bool LSP_MODE = false;

/// CXIRCompiler compiler;
/// compiler.compile_CXIR(CXXCompileAction::init(emitter, out, flags, cxx_args));
/// NOTE: init returns a rvalue reference you can not assign it to a variable
struct CXXCompileAction {
    using Path = std::filesystem::path;
    using Args = std::vector<std::string>;
    using CXIR = generator::CXIR::CXIR;

    Path               working_dir;
    Path               cc_source;
    Path               cc_output;
    Path               helix_src;
    Args               cxx_args;
    flag::CompileFlags flags;
    std::string        cxx_compiler;

    static CXXCompileAction
    init(CXIR &emitter, const Path &cc_out, flag::CompileFlags flags, Args cxx_args) {
        std::error_code ec;
        Path            cwd      = __CONTROLLER_FS_N::get_cwd();
        Path            temp_dir = std::filesystem::temp_directory_path(ec);

        if (ec) {  /// use the current working directory
            temp_dir = cwd;
        }

        CXXCompileAction action = {
            .working_dir = cwd,
            .cc_source   = temp_dir / generate_file_name(),
            .cc_output   = cc_out,
            .helix_src   = emitter.get_file_name().value_or(cc_out),
            .cxx_args    = std::move(cxx_args),
            .flags       = flags,
#if IS_UNIX
            .cxx_compiler = "c++",
#else
            .cxx_compiler = "",  // find msvc using vswhere, if not found try `c++`
#endif
        };

        std::ofstream file(action.cc_source);

        if (!file) {
            helix::log<LogLevel::Error>("error creating ", action.cc_source.generic_string(), " file");
            return action;
        }

        file << emitter.to_CXIR();
        file.close();

        if (flags.contains(EFlags(flag::types::CompileFlags::Verbose))) {
            helix::log<LogLevel::Debug>("CXXCompileAction initialized with:");
            helix::log<LogLevel::Debug>("working_dir: ", action.working_dir.generic_string());
            helix::log<LogLevel::Debug>("cc_source: ", action.cc_source.generic_string());
            helix::log<LogLevel::Debug>("cc_output: ", action.cc_output.generic_string());
            helix::log<LogLevel::Debug>("helix_src: ", action.helix_src.generic_string());
            helix::log<LogLevel::Debug>("cxx_compiler: ", action.cxx_compiler);
            helix::log<LogLevel::Debug>(
                "cxx_args: ",
                "[" +
                    std::accumulate(action.cxx_args.begin(),
                                    action.cxx_args.end(),
                                    std::string(),
                                    [](const std::string &a, const std::string &b) {
                                        return a.empty() ? b : a + ", " + b;
                                    }) +
                    "]");
        }

        return action;
    }

    ~CXXCompileAction() {
        if (std::filesystem::exists(cc_source)) {
            std::filesystem::remove(cc_source);
        }
    }

  private:
    static std::string generate_file_name(size_t length = 6) {
        const std::string chars = "QCDEGHINOPQRSAIUVYZabcefgjklnopqrsuvwyz012345789";
        
        std::random_device              rd;
        std::mt19937                    gen(rd());
        std::string                     name = "__";
        std::uniform_int_distribution<> dist(0, chars.size() - 1);

        name.reserve(length + name.size());

        std::generate_n(std::back_inserter(name), length, [&]() { return chars[dist(gen)]; });

        return name + ".helix-compiler.cxir";
    }
};

class CXIRCompiler {
  public:
    struct ExecResult {
        std::string output;
        int         return_code{};
    };

    using CompileResult = std::pair<ExecResult, flag::ErrorType>;

    [[nodiscard]] static ExecResult exec(const std::string &cmd);

    void compile_CXIR(CXXCompileAction &&action) const {
#if IS_WIN32
        // try compiling with msvc first
        if (action.cxx_compiler.empty()) {
            if (CXIR_MSVC(action).second == flag::types::ErrorType::NotFound) {
                // try compiling with clang
                action.cxx_compiler = "c++";
                CXIR_CXX(action);
            }

            return;
        } else {
            CXIR_CXX(action);
        }
#else
        CXIR_CXX(action);
#endif
    }

  private:
    /// (pof, msg, file)
    using ErrorPOFNormalized = std::tuple<token::Token, std::string, std::string>;

    [[nodiscard]] static ErrorPOFNormalized parse_clang_err(std::string clang_out);
    [[nodiscard]] static ErrorPOFNormalized parse_gcc_err(std::string gcc_out);
    [[nodiscard]] static ErrorPOFNormalized parse_msvc_err(std::string msvc_out);

    [[nodiscard]] CompileResult CXIR_MSVC(CXXCompileAction action) const;

    [[nodiscard]] CompileResult CXIR_CXX(CXXCompileAction action) const;
};

class CompilationUnit {
  public:
    int compile(int argc, char **argv);

  private:
    CXIRCompiler compiler;

    static void remove_comments(__TOKEN_N::TokenList &tokens);

    static void emit_cxir(const generator::CXIR::CXIR &emitter, bool verbose);

    static std::filesystem::path
    determine_output_file(const __CONTROLLER_CLI_N::CLIArgs &parsed_args,
                          const std::filesystem::path       &in_file_path);

    static void log_time(const std::chrono::high_resolution_clock::time_point &start,
                         bool                                                  verbose,
                         const std::chrono::high_resolution_clock::time_point &end);
};

#endif  // __TOOLING_H__