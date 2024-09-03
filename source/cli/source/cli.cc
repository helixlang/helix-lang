//===------------------------------------------------------------------------------------------===//
//
// Part of the Helix Project, under the Attribution 4.0 International license (CC BY 4.0).
// You are allowed to use, modify, redistribute, and create derivative works, even for commercial
// purposes, provided that you give appropriate credit, and indicate if changes were made.
// For more information, please visit: https://creativecommons.org/licenses/by/4.0/
//
// SPDX-License-Identifier: CC-BY-4.0
// Copyright (c) 2024 (CC BY 4.0)
//
//===------------------------------------------------------------------------------------------===//
/*
Helios - package manager / build system

Usage:
    helios create <project>               Create a new Helix project.
    helios build                          Build the current project.
    helios run [-r] -- <args>             Run the current project with optional arguments.
    helios test                           Run tests for the current project.
    helios clean                          Clean build artifacts.
    helios install <package>              Install a package.
    helios uninstall <package>            Uninstall a package.
    helios update                         Update all installed packages.
    helios search <package>               Search for a package.
    helios list                           List all installed packages.
    helios info <package>                 Show information about a specific package.
    helios help                           Show help information.
    helios version                        Show the version of Helios.
    helios license                        Show the license of Helios.
    helios doc-gen                        Generate documentation.
*/

/*
helix - command line interface

Usage:
    helix [options] <file> [options-2]
    helix (-h | --help)
    helix --version
    helix --license

options:
    -O1 --optimize1          Optimization level 1.
    -O2 --optimize2          Optimization level 2.
    -O3 --optimize3          Optimization level 3.
    -O4 --optimize4          Optimization level 4.
    -O5 --optimize5          Optimization level 5.

    -h --help                Show this screen.
    -v --version             Show version.
    -l --license             Show license information.

    --verbose                Show verbose output.
    --quiet                  Show no output.

    --emit-tokens            Show tokens.
    --emit-llvm              Show LLVM IR.
    --emit-asm               Show assembly.
    --emit-ast               Show AST in json format
    --emit-ir                Show intermediate representation. (C++)
    --emit-doc               Only extract doc-comments along with signatures in json format.

    --toolchain <options-3>  Set the toolchain to use

    --config <file>          Specify configuration file.
    -r --release             Build in release mode.
    -d --debug               Build in debug mode with symbols.

options-2:
    --lib [abi-options]      Compile as library.
    -o <file>                Output file.
    -I <dir>                 Include directory.
    -L <dir>                 Library directory.
    -l <lib>                 Link library.
    -m <mod>                 Helix Modules directory.

options-3:
    --target <triple>        Target triple.
    --arch   <arch>          Target architecture.
    --os     <os>            Target operating system.
    --sdk    <sdk-path>      Optional Path to a sdk.

abi-options:
    -py  --python        Python stub files gen and pylib compile
    -rs  --rust          Rust source files gen and a rust compiler lib
    -cxx --cxx           C++ header files gen and linkable object file
    -hlx --helix         Helix ABI compatable libary
*/

#include "cli/include/cli.hh"

#include <cstddef>
#include <iostream>
#include <optional>
#include <string>

#include "cli/include/args.hh"
#include "core/utils/colors_ansi.hh"

namespace command_line {
CLIArgs::CLIArgs(int argc, char **argv, const std::string &_version) {
    args::ArgumentParser parser("Helix Compiler Command Line Interface", R"(
Helix CLI - The Next-Generation Compiler for Modern Development

Usage Examples:
  1. Compile a Helix source file:
     helix input_file.hlx

  2. Compile with optimization level 2:
     helix -O2 -o output_file input_file.hlx

  3. Generate LLVM Intermediate Representation (IR):
     helix --emit-llvm -o output.ll input_file.hlx

  4. Cross-compile for a specific target:
     helix --target x86_64-linux-gnu -o output_file input_file.hlx

  5. Compile as a library with Rust ABI:
     helix --lib --rust -o liboutput.a input_file.hlx

For more detailed documentation, visit:
  https://helix-lang.com/docs

For support or to report issues, contact:
  support@helix-lang.com

Thank you for using Helix!
)");
    args::HelpFlag       help(parser, "help", "Display this help menu", {'h', "help"});
    args::Flag version(parser, "version", "Display the version information", {'v', "version"});
    args::Flag license(parser, "license", "Display license information", {"license"});

    args::Flag verbose(parser, "verbose", "Enable verbose output", {"verbose"});
    args::Flag quiet(parser, "quiet", "Suppress all output", {"quiet"});

    args::Flag emit_tokens(parser, "emit-tokens", "Output tokenized source code", {"emit-tokens"});
    args::Flag emit_llvm(
        parser, "emit-llvm", "Output LLVM Intermediate Representation (IR)", {"emit-llvm"});
    args::Flag emit_asm(parser, "emit-asm", "Output assembly code", {"emit-asm"});
    args::Flag emit_ast(
        parser, "emit-ast", "Output Abstract Syntax Tree (AST) in JSON format", {"emit-ast"});
    args::Flag emit_cst(
        parser, "emit-cst", "Output Concrete Syntax Tree (CST) in JSON format", {"emit-cst"});
    args::Flag emit_ir(parser, "emit-ir", "Output intermediate representation (C++)", {"emit-ir"});
    args::Flag emit_doc(parser,
                        "emit-doc",
                        "Extract and output doc-comments along with signatures in JSON format",
                        {"emit-doc"});

    args::Group toolchain_group(
        parser, "Cross Compilation Toolchain Options", args::Group::Validators::AtMostOne);
    args::ValueFlag<std::string> toolchain_target(
        parser, "target", "Specify target triple for cross-compilation", {"target"});
    args::ValueFlag<std::string> toolchain_arch(
        parser, "arch", "Specify target architecture", {"arch"});
    args::ValueFlag<std::string> toolchain_cpu(parser, "cpu", "Specify target MCPU", {"cpu"});
    args::ValueFlag<std::string> toolchain_sdk(
        parser, "sdk", "Specify optional path to SDK", {"sdk"});

    args::ValueFlag<std::string> config_file(
        parser, "config", "Specify path to configuration file", {"config"});
    args::ValueFlag<std::string> output_file(parser, "output", "Specify output file path", {'o'});

    args::Flag release(
        parser, "release", "Build in release mode with optimizations", {'r', "release"});
    args::Flag debug(parser, "debug", "Build in debug mode with debugging symbols", {'d', "debug"});

    args::Group optimization_group(
        parser, "Optimization Levels", args::Group::Validators::AtMostOne);
    args::Flag optimize1(
        optimization_group, "optimize1", "Enable optimization level 1", {"O1", "optimize1"});
    args::Flag optimize2(
        optimization_group, "optimize2", "Enable optimization level 2", {"O2", "optimize2"});
    args::Flag optimize3(
        optimization_group, "optimize3", "Enable optimization level 3", {"O3", "optimize3"});
    args::Flag optimize4(
        optimization_group, "optimize4", "Enable optimization level 4", {"O4", "optimize4"});
    args::Flag optimize5(
        optimization_group, "optimize5", "Enable optimization level 5", {"O5", "optimize5"});

    args::Flag lib(parser, "lib", "Compile as a library", {"lib"});

    args::Group abi_group(parser, "ABI Options", args::Group::Validators::AtMostOne);
    args::Flag  python_abi(abi_group,
                          "python",
                          "Generate Python stub files and compile to a Python library",
                           {"py", "python"});
    args::Flag  rust_abi(abi_group,
                        "rust",
                        "Generate Rust source files and compile to a Rust library",
                         {"rs", "rust"});
    args::Flag  cxx_abi(abi_group,
                       "cxx",
                       "Generate C++ header files and compile to a linkable object file",
                        {"cxx", "cxx"});
    args::Flag  helix_abi(
        abi_group, "helix", "Generate Helix ABI-compatible library", {"hlx", "helix"});

    args::ValueFlagList<std::string> include_dirs(
        parser, "include", "Specify include directories", {'I'});
    args::ValueFlagList<std::string> library_dirs(
        parser, "libdir", "Specify library directories", {'L'});
    args::ValueFlagList<std::string> link_libraries(
        parser, "lib", "Specify libraries to link against", {'l'});
    args::ValueFlagList<std::string> module_dirs(
        parser, "moddir", "Specify Helix modules directories", {'m'});
    args::Positional<std::string> input_file(parser, "file", "Specify input file path");

    parser.LongSeparator(" ");

    try {
        parser.ParseCLI(argc, argv);

        if (help) {
            std::cout << parser;
            this->exit_   = true;
            this->exit_co = 0;
            return;
        }
        if (version) {
            std::cout << "Helix Compiler Version: " << _version << std::endl;
            this->exit_   = true;
            this->exit_co = 0;
            return;
        }
        if (license) {
            std::cout << R"(
Helix Language Project - License Information

This software is part of the Helix Language Project and is licensed under the Attribution 4.0 International license (CC BY 4.0).

You are permitted to:
  - Use, modify, and redistribute the software
  - Create derivative works, even for commercial purposes

Provided that you:
  - Give appropriate credit
  - Provide a link to the license
  - Indicate if changes were made

For more detailed information, please visit: https://creativecommons.org/licenses/by/4.0/

SPDX-License-Identifier: CC-BY-4.0

This software is provided by the creators of Helix. Visit our website at:
https://helix-lang.com/ for more information.
    )" << std::endl;
            this->exit_   = true;
            this->exit_co = 0;
            return;
        }

        this->file        = args::get(input_file);
        this->output_file = output_file ? std::make_optional(args::get(output_file)) : std::nullopt;

        if (optimize1) {
            optimize = OPTIMIZATION::O1;
        }
        if (optimize2) {
            optimize = OPTIMIZATION::O2;
        }
        if (optimize3) {
            optimize = OPTIMIZATION::O3;
        }
        if (optimize4) {
            optimize = OPTIMIZATION::O4;
        }
        if (optimize5) {
            optimize = OPTIMIZATION::O5;
        }

        this->help        = help;
        this->verbose     = verbose;
        this->quiet       = quiet;
        this->emit_tokens = emit_tokens;
        this->emit_llvm   = emit_llvm;
        this->emit_asm    = emit_asm;
        this->emit_ast    = emit_ast;
        this->emit_cst    = emit_cst;
        this->emit_ir     = emit_ir;
        this->emit_doc    = emit_doc;

        if (verbose && quiet) {
            std::cerr << colors::fg16::red << "Error:" << colors::reset
                      << " Cannot specify both verbose and quiet options." << '\n';
            this->exit_   = true;
            this->exit_co = 1;
            return;
        }

        if (verbose) {
            std::cout << colors::fg16::red << "Verbose output enabled." << colors::reset << '\n';
        }

        if (release) {
            build_mode = MODE::RELEASE;
        }
        if (debug) {
            build_mode = MODE::DEBUG_;
        }

        if (lib) {
            if (python_abi) {
                build_lib = ABI::PYTHON;
            } else if (rust_abi) {
                build_lib = ABI::RUST;
            } else if (cxx_abi) {
                build_lib = ABI::CXX;
            } else {
                build_lib = ABI::HELIX;
            }
        }

        if (toolchain_target) {
            toolchain.target = args::get(toolchain_target);
        }
        if (toolchain_arch) {
            toolchain.arch = args::get(toolchain_arch);
        }
        if (toolchain_cpu) {
            toolchain.cpu = args::get(toolchain_cpu);
        }
        if (toolchain_sdk) {
            toolchain.sdk = args::get(toolchain_sdk);
        }

        if (config_file) {
            this->config_file = args::get(config_file);
        }

        this->include_dirs   = args::get(include_dirs);
        this->library_dirs   = args::get(library_dirs);
        this->link_libraries = args::get(link_libraries);
        this->module_dirs    = args::get(module_dirs);

        this->get_all_flags += "Flags: \n";
        this->get_all_flags += "    Verbose: "           + std::to_string(verbose) + ", \n";
        this->get_all_flags += "    Quiet: "             + std::to_string(quiet) + ", \n";
        this->get_all_flags += "    Emit Tokens: "       + std::to_string(emit_tokens) + ", \n";
        this->get_all_flags += "    Emit LLVM: "         + std::to_string(emit_llvm) + ", \n";
        this->get_all_flags += "    Emit ASM: "          + std::to_string(emit_asm) + ", \n";
        this->get_all_flags += "    Emit AST: "          + std::to_string(emit_ast) + ", \n";
        this->get_all_flags += "    Emit CST: "          + std::to_string(emit_cst) + ", \n";
        this->get_all_flags += "    Emit IR: "           + std::to_string(emit_ir) + ", \n";
        this->get_all_flags += "    Emit Doc: "          + std::to_string(emit_doc) + ", \n";
        this->get_all_flags += "    Release: "           + std::to_string(release) + ", \n";
        this->get_all_flags += "    Debug: "             + std::to_string(debug) + ", \n";
        this->get_all_flags += "    Lib: "               + std::to_string(lib) + ", \n";
        this->get_all_flags += "    Python ABI: "        + std::to_string(python_abi) + ", \n";
        this->get_all_flags += "    Rust ABI: "          + std::to_string(rust_abi) + ", \n";
        this->get_all_flags += "    CXX ABI: "           + std::to_string(cxx_abi) + ", \n";
        this->get_all_flags += "    Helix ABI: "         + std::to_string(helix_abi) + ", \n";

        this->get_all_flags += "    Config File: "       + config_file.Get() + ", \n";
        this->get_all_flags += "    Output File: "       + output_file.Get() + ", \n";
        this->get_all_flags += "    Toolchain Target: "  + toolchain_target.Get() + ", \n";
        this->get_all_flags += "    Toolchain Arch: "    + toolchain_arch.Get() + ", \n";
        this->get_all_flags += "    Toolchain CPU: "     + toolchain_cpu.Get() + ", \n";
        this->get_all_flags += "    Toolchain SDK: "     + toolchain_sdk.Get() + ", \n";

        this->get_all_flags += "    Include Dir: ["      + std::string(!include_dirs.Get().empty() ? "\n" : " ");;
        for (auto &dir : include_dirs.Get()) {
            this->get_all_flags += dir + ", \n    ";
        }

        this->get_all_flags += "    ], \n";
        this->get_all_flags += "    Library Dir: [" + std::string(!library_dirs.Get().empty() ? "\n" : " ");
        for (auto &dir : library_dirs.Get()) {
            this->get_all_flags += dir + ", \n    ";
        }

        this->get_all_flags += "    ], \n";
        this->get_all_flags += "    Link Libraries: [" + std::string(!link_libraries.Get().empty() ? "\n" : " ");
        for (auto &lib : link_libraries.Get()) {
            this->get_all_flags += lib + ", \n    ";
        }

        this->get_all_flags += "    ], \n";
        this->get_all_flags += "    Module Dir: [" + std::string(!module_dirs.Get().empty() ? "\n" : " ");
        for (auto &mod : module_dirs.Get()) {
            this->get_all_flags += mod + ", \n    ";
        }

        this->get_all_flags += "    ]";

    } catch (args::Help) { std::cout << parser; } catch (args::ParseError &e) {
        std::cerr << e.what() << std::endl << parser;
        this->exit_   = true;
        this->exit_co = 1;
        return;
    } catch (args::ValidationError &e) {
        std::cerr << e.what() << std::endl << parser;
        this->exit_   = true;
        this->exit_co = 1;
        return;
    }
}
}  // end namespace command_line