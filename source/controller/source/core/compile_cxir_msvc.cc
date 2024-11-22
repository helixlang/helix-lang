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

#include <filesystem>

#include "controller/include/config/Controller_config.def"
#include "controller/include/config/cxx_flags.hh"
#include "controller/include/shared/eflags.hh"
#include "controller/include/shared/file_system.hh"
#include "controller/include/shared/logger.hh"
#include "controller/include/tooling/tooling.hh"
#include "parser/preprocessor/include/preprocessor.hh"
#include "token/include/config/Token_config.def"

#ifndef DEBUG_LOG
#define DEBUG_LOG(...)                            \
    if (is_verbose) {                             \
        helix::log<LogLevel::Debug>(__VA_ARGS__); \
    }
#endif

CXIRCompiler::CompileResult CXIRCompiler::CXIR_MSVC(const CXXCompileAction &action) const {
    bool is_verbose       = action.flags.contains(EFlags(flag::types::CompileFlags::Verbose));
    bool vswhere_found    = false;
    bool msvc_found       = false;
    bool msvc_tools_found = false;

    ExecResult            compile_result;
    ExecResult            vs_result;
    std::filesystem::path msvc_tools_path;

    std::string vs_path;
    std::string compile_cmd;

    std::string where_vswhere =
        R"("C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" )";

    std::string vswhere_cmd =
        where_vswhere +
        "-latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property "
        "installationPath";

    DEBUG_LOG("vswhere command: " + vswhere_cmd);
    vs_result = exec(vswhere_cmd);

    if (!std::filesystem::exists(
            "C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe")) {
        DEBUG_LOG("vswhere not found");
        return {vs_result, flag::ErrorType(flag::types::ErrorType::NotFound)};
    }

    if (vs_result.return_code != 0) {
        DEBUG_LOG("vswhere failed to execute: " + std::to_string(vs_result.return_code));
        DEBUG_LOG("vswhere output: " + vs_result.output);
        return {vs_result, flag::ErrorType(flag::types::ErrorType::NotFound)};
    }

    DEBUG_LOG("vswhere output: " + vs_result.output);
    vs_path = vs_result.output;
    vs_path.erase(vs_path.find_last_not_of(" \n\r\t") + 1);  // trim trailing whitespace

    DEBUG_LOG("vswhere path: " + vs_path);
    vswhere_found = vs_result.return_code == 0 && !vs_result.output.empty();
    msvc_found    = std::filesystem::exists(vs_path);

    if (!vswhere_found || !msvc_found) {
        helix::log<LogLevel::Warning>(
            "visual Studio not found attempting to find any other c++ compiler");
        return {vs_result, flag::ErrorType(flag::types::ErrorType::NotFound)};
    }

    msvc_tools_path =
        std::filesystem::path(vs_path) / "VC" / "Auxiliary" / "Build" / "vcvars64.bat";
    msvc_tools_found = std::filesystem::exists(msvc_tools_path);

    if (!msvc_tools_found) {
        helix::log<LogLevel::Warning>(
            "msvc tools not found attempting to find any other c++ compiler");
        return {vs_result, flag::ErrorType(flag::types::ErrorType::NotFound)};
    }

    DEBUG_LOG("msvc tools path: " + msvc_tools_path.generic_string());
    compile_cmd = "cmd.exe /c \"call \"" + msvc_tools_path.string() + "\" >nul 2>&1 && cl ";

    // get the path to the core lib
    auto core = __CONTROLLER_FS_N::get_exe().parent_path().parent_path() / "core" / "include" / "core.h";

    if (!std::filesystem::exists(core)) {
        helix::log<LogLevel::Error>("core lib not found, verify the installation");
        return {compile_result, flag::ErrorType(flag::types::ErrorType::NotFound)};
    }

    /// start with flags we know are going to be present
    compile_cmd += make_command(  // ...
        flag::types::Compiler::MSVC,

        "/FI\"" + core.generic_string() + "\" ",
        ((action.flags.contains(flag::types::CompileFlags::Debug))
             ? cxx::flags::debugModeFlag
             : cxx::flags::optimizationLevel3),

        cxx::flags::cxxStandardFlag,
        cxx::flags::stdCXX23Flag,
        cxx::flags::enableExceptionsFlag,

        "/nologo",
        "/Zc:__cplusplus",
        ((action.flags.contains(flag::types::CompileFlags::Debug)) ? "/RTC1" : ""),


        // cxx::flags::noDefaultLibrariesFlag,
        // cxx::flags::noCXXSTDLibrariesFlag,
        // cxx::flags::noCXXSTDIncludesFlag,
        // cxx::flags::noBuiltinIncludesFlag,
        // FIXME: add these later
        // cxx::flags::linkTimeOptimizationFlag,
        cxx::flags::warnAllFlag,
        cxx::flags::outputFlag,
        "\"" + action.cc_output.generic_string() + "\""  // output
    );

    if (this->dry_run) {
        compile_cmd += std::string(cxx::flags::dryRunFlag.msvc) + " ";
    }

    /// add any additional flags passed into the action
    for (auto &flag : action.cxx_args) {
        compile_cmd += flag + " ";
    }

    if (!COMPILE_ACTIONS.empty()) {
        for (auto &action : COMPILE_ACTIONS) {
            compile_cmd += "\"" + action.cc_source.generic_string() + "\" ";
        }
    }

    /// add the source file in a normalized path format
    compile_cmd += "\"" + action.cc_source.generic_string() + "\"";

    DEBUG_LOG("compile command: " + compile_cmd);

    if (!std::filesystem::exists(action.cc_source)) {
        helix::log<LogLevel::Error>(
            "source file has been removed or does not exist, possible memory corruption");
        return {compile_result, flag::ErrorType(flag::types::ErrorType::Error)};
    }

    /// execute the command
    compile_result = exec(compile_cmd);

    DEBUG_LOG("compile command: " + compile_cmd);
    DEBUG_LOG("compiler output:\n" + compile_result.output);

    // if the compile outputted an obj file in the cwd remove it.
    std::filesystem::path obj_p =
        __CONTROLLER_N::file_system::get_cwd() / action.cc_source.filename();
    obj_p.replace_extension(".obj");

    if (std::filesystem::exists(obj_p)) {
        std::error_code ec_remove;
        std::filesystem::remove(obj_p, ec_remove);

        if (ec_remove) {
            helix::log<LogLevel::Warning>("failed to delete obj " + obj_p.generic_string() + ": " +
                                          ec_remove.message());
        }
    }

    /// parse the output and show errors after translating them to helix errors
    std::vector<std::string> lines;
    std::istringstream       stream(compile_result.output);

    DEBUG_LOG("parsing compiler output");
    for (std::string line; std::getline(stream, line);) {
        lines.push_back(line);
    }

    for (auto & line : lines) {
         ErrorPOFNormalized err = CXIRCompiler::parse_msvc_err(line);

        if (std::get<0>(err).token_kind() == __TOKEN_N::WHITESPACE) {
            continue;
        }

        if (!std::filesystem::exists(std::get<2>(err))) {
            if (std::get<2>(err).empty()) {
                continue;
            }

            continue;

            // FIXME: for some fucking reason this causes some sort of memory corruption
            //        and caused the compiler to exit prematurely with a exit code of 0
            //        this is a temporary fix until I can figure out what is causing it
            //        to crash, this shit took 3 fucking hours of debugging to figure out.
            error::Panic _(error::CompilerError{
                .err_code     = 0.8245,
                .err_fmt_args = {"error at: " + std::get<2>(err) + std::get<1>(err)},
            });

            continue;
        }

        std::pair<size_t, size_t> err_t = {std::get<1>(err).find_first_not_of(' '),
                                           std::get<1>(err).find('C') -
                                               std::get<1>(err).find_first_not_of(' ') - 1};

        error::Level level = std::map<string, error::Level>{
            {"error", error::Level::ERR},                       //
            {"warning", error::Level::WARN},                    //
            {"note", error::Level::NOTE}                        //
        }[std::get<1>(err).substr(err_t.first, err_t.second)];  //

        auto pof = std::get<0>(err);
        try {  // if the file is a core lib file, ignore all but errors
            std::string f_name =
                std::filesystem::path(std::get<2>(err)).filename().generic_string();

            if ((f_name.size() == 28 && f_name.substr(9, 19) == "helix-compiler.cxir")) {
                if (level != error::Level::ERR) {
                    continue;
                }

                // if it is error rename pof file to helix core
                pof.set_file_name(pof.file_name() + "$helix.core.lib");
            }
        } catch (...) {}

        std::string msg = std::get<1>(err).substr(err_t.first + err_t.second + 1);

        error::Panic(error::CodeError{
            .pof          = &pof,
            .err_code     = 0.8245,
            .mark_pof     = false,
            .err_fmt_args = {msg.substr(msg.find_first_not_of(' '))},
            .level        = level,
            .indent       = static_cast<size_t>((level == error::NOTE) ? 1 : 0),
        });
    }

    if (compile_result.return_code == 0) {
        helix::log<LogLevel::Info>("lowered " + action.helix_src.generic_string() +
                                   " and compiled cxir");
        helix::log<LogLevel::Info>("compiled successfully to " + action.cc_output.generic_string());

        return {compile_result, flag::ErrorType(flag::types::ErrorType::Success)};
    }

    return {compile_result,
            flag::ErrorType(error::HAS_ERRORED ? flag::types::ErrorType::Error
                                               : flag::types::ErrorType::Success)};
}
