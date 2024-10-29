#include "controller/include/shared/logger.hh"
#include "controller/include/tooling/tooling.hh"

CXIRCompiler::CompileResult CXIRCompiler::CXIR_MSVC(CXXCompileAction action) const {

    std::string where_vswhere =
        R"("C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" )";
    std::string vswhere_cmd =
        where_vswhere +
        "-latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property "
        "installationPath";

    // msvc_compiler: {
    //     std::filesystem::path msvc_tools_path;
    //     ExecResult            vs_result = exec(vswhere_cmd);

    //     std::string vs_path = vs_result.output;
    //     vs_path.erase(vs_path.find_last_not_of(" \n\r\t") + 1);  // trim trailing whitespace
    //     (win32)

    //     bool vswhere_found = vs_result.return_code == 0 && !vs_result.output.empty();
    //     bool msvc_found    = std::filesystem::exists(vs_path);
    //     bool msvc_tools_found;

    //     if (!vswhere_found || !msvc_found) {
    //         helix::log<LogLevel::Warning>("visual Studio not found attempting to find any other c++
    //         compiler"); goto mingw_compiler;
    //     }

    //     msvc_tools_path =
    //         std::filesystem::path(vs_path) / "VC" / "Auxiliary" / "Build" / "vcvars64.bat";
    //     msvc_tools_found = std::filesystem::exists(msvc_tools_path);

    //     if (!msvc_tools_found) {
    //         helix::log<LogLevel::Warning>("msvc tools not found attempting to find any other c++
    //         compiler"); goto mingw_compiler;
    //     }

    //     std::string compile_cmd = "cmd.exe /c \"";
    //     compile_cmd += "call \"" + msvc_tools_path.string() + "\" >nul 2>&1 && ";
    //     compile_cmd += "cl /std:c++latest ";                  // Use C++23
    //     compile_cmd += is_debug ? "/Zi " : "/O2 ";            // Debug or Release
    //     compile_cmd += "/EHsc ";                              // Enable C++ exceptions
    //     compile_cmd += "/Fe:" + (paths.second / out).string() + " ";  // Output file
    //     compile_cmd += "\"" + paths.first.string() + "\"";    // Source file
    //     compile_cmd += "\"";

    //     ExecResult compile_result = exec(compile_cmd);

    //     if (compile_result.return_code != 0) {
    //         helix::log<LogLevel::Error>("compilation failed with return code " +
    //                              std::to_string(compile_result.return_code));
    //         helix::log<LogLevel::Error>("compiler output:\n" + compile_result.output);
    //         goto cleanup;
    //     }

    //     helix::log<LogLevel::Info>("compiled cxir from " + paths.first.string());
    //     helix::log<LogLevel::Info>("compiled successfully to " + (paths.second / out).string());
    // }

    // cleanup: {
    //     std::filesystem::remove(paths.first);
    //     std::filesystem::path obj_file = paths.first;
    //     obj_file.replace_extension(".obj");

    //     if (std::filesystem::exists(obj_file)) {
    //         std::error_code ec_remove;
    //         std::filesystem::remove(obj_file, ec_remove);

    //         if (ec_remove) {
    //             helix::log<LogLevel::Warning>("failed to delete object file " + obj_file.string() + ": "
    //             +
    //                                    ec_remove.message());
    //         } else {
    //             helix::log<LogLevel::Info>("deleted object file " + obj_file.string());
    //         }
    //         return;
    //     }

    //     obj_file = paths.second / paths.first.filename();
    //     obj_file.replace_extension(".obj");

    //     if (std::filesystem::exists(obj_file)) {
    //         std::error_code ec_remove;
    //         std::filesystem::remove(obj_file, ec_remove);

    //         if (ec_remove) {
    //             helix::log<LogLevel::Warning>("failed to delete object file " + obj_file.string() + ": "
    //             +
    //                                    ec_remove.message());
    //         } else {
    //             helix::log<LogLevel::Info>("deleted object file " + obj_file.string());
    //         }
    //     }

    //     return;
    // }

    return {{"", 0}, flag::ErrorType(flag::types::ErrorType::Success)};
}