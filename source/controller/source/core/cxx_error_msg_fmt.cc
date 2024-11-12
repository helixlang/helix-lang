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

#include <unordered_map>
#include "controller/include/tooling/tooling.hh"

/// key is the line number in the output, value is the line and column, and length associated in the source
 std::unordered_map<size_t, std::tuple<size_t, size_t, size_t>> LINE_COLUMN_MAP;

CXIRCompiler::ErrorPOFNormalized CXIRCompiler::parse_clang_err(std::string clang_out) {
    std::string filePath;
    size_t         lineNumber   = 0;
    size_t         columnNumber = 0;
    size_t         length       = 0;
    std::string message;

    std::istringstream stream(clang_out);
    std::getline(stream, filePath, ':');  // Extract file path
    stream >> lineNumber;                               // Extract line number
    stream.ignore();                                    // Ignore the next colon
    stream >> columnNumber;                             // Extract column number
    stream.ignore();                                    // Ignore the next colon
    std::getline(stream, message);            // Extract the message

    if (LINE_COLUMN_MAP.find(lineNumber) != LINE_COLUMN_MAP.end()) {
        auto source_loc = LINE_COLUMN_MAP[lineNumber];
        lineNumber     = std::get<0>(source_loc);
        columnNumber   = std::get<1>(source_loc);
        length         = std::get<2>(source_loc);
    }

    token::Token pof = token::Token(lineNumber, columnNumber, length, columnNumber+lineNumber, "/*error*/", filePath, "<other>");

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