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

#include "controller/include/tooling/tooling.hh"

CXIRCompiler::ErrorPOFNormalized CXIRCompiler::parse_clang_err(std::string clang_out) {
    std::string filePath;
    int         lineNumber   = 0;
    int         columnNumber = 0;
    std::string message;

    std::istringstream stream(clang_out);
    std::getline(stream, filePath, ':');  // Extract file path
    stream >> lineNumber;                 // Extract line number
    stream.ignore();                      // Ignore the next colon
    stream >> columnNumber;               // Extract column number
    stream.ignore();                      // Ignore the next colon
    std::getline(stream, message);        // Extract the message

    token::Token pof =
        token::Token(lineNumber, 0, 1, columnNumber, "/*error*/", filePath, "<other>");

    return {pof, message, filePath};
}

CXIRCompiler::ErrorPOFNormalized CXIRCompiler::parse_gcc_err(std::string gcc_out) {
    return parse_clang_err(gcc_out);
}
CXIRCompiler::ErrorPOFNormalized CXIRCompiler::parse_msvc_err(std::string msvc_out) {
    std::string filePath;
    int         lineNumber = 0;
    std::string message;

    std::istringstream stream(msvc_out);
    std::getline(stream, filePath, '(');  // Extract file path

    bool isFile = false;

    try {
        std::filesystem::path path(filePath);
        isFile = !path.empty();
    } catch (...) { isFile = false; }

    if (!isFile) {
        return {token::Token(), "", ""};
    }

    stream >> lineNumber;           // Extract line number
    stream.ignore();                // Ignore the next bracket
    stream.ignore();                // Ignore the next colon
    std::getline(stream, message);  // Extract the message

    token::Token pof = token::Token(lineNumber, 0, 1, 1, "/*error*/", filePath, "<other>");

    return {pof, message, filePath};
}