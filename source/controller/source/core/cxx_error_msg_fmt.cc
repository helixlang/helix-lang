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
#include "generator/include/CX-IR/loc.hh"

/// key is the line number in the output, value is the line and column, and length associated in the
/// source

CXIRCompiler::ErrorPOFNormalized CXIRCompiler::parse_clang_err(std::string clang_out) {
    std::string filePath;
    size_t      lineNumber   = 0;
    size_t      columnNumber = 0;
    size_t      length       = 0;
    std::string message;

    std::istringstream stream(clang_out);
    std::getline(stream, filePath, ':');  // Extract file path
    stream >> lineNumber;                 // Extract line number
    stream.ignore();                      // Ignore the next colon
    stream >> columnNumber;               // Extract column number
    stream.ignore();                      // Ignore the next colon
    std::getline(stream, message);        // Extract the message

    // see if filepath is in std::unordered_map<std::string, SourceMap> SOURCE_MAPS
    // if it is, call SOURCE_MAPS[filePath].get_pof(lineNumber, columnNumber, length)

    if (filePath.empty()) {
        return {token::Token(), "", ""};
    }

    if (generator::CXIR::SOURCE_MAPS.find(filePath) != generator::CXIR::SOURCE_MAPS.end()) {
        generator::CXIR::SourceMap     &source_map = generator::CXIR::SOURCE_MAPS[filePath];
        generator::CXIR::SourceLocation loc        = source_map.find_loc(lineNumber, columnNumber);
        if (!loc.is_placeholder) {
            lineNumber   = loc.line;
            columnNumber = loc.column;
            length       = loc.length;
        } else {
            return {token::Token(), "", ""};
        }
    }

    token::Token pof = token::Token(lineNumber,
                                    columnNumber,
                                    length,
                                    columnNumber + lineNumber,
                                    "/*error*/",
                                    filePath,
                                    "<other>");

    return {pof, message, filePath};
}

CXIRCompiler::ErrorPOFNormalized CXIRCompiler::parse_gcc_err(std::string gcc_out) {
    return parse_clang_err(gcc_out);
}
CXIRCompiler::ErrorPOFNormalized CXIRCompiler::parse_msvc_err(std::string msvc_out) {
    std::string filePath;
    size_t      lineNumber = 0;
    size_t      columnNumber = 0;
    std::string message;

    std::istringstream stream(msvc_out);
    if (!std::getline(stream, filePath, '(')) {
        return {token::Token(), "", ""};
    }

    bool isFile = false;

    try {
        std::filesystem::path path(filePath);
        isFile = !path.empty();
    } catch (...) { isFile = false; }

    if (!isFile) {
        return {token::Token(), "", ""};
    }

    stream >> lineNumber;           // Extract line number
    // fixme: make msvc output column number
    stream.ignore();                // Ignore the next bracket
    stream.ignore();                // Ignore the next colon
    std::getline(stream, message);  // Extract the message

    token::Token pof;

    if (generator::CXIR::SOURCE_MAPS.find(filePath) != generator::CXIR::SOURCE_MAPS.end()) {
        generator::CXIR::SourceMap     &source_map = generator::CXIR::SOURCE_MAPS[filePath];
        generator::CXIR::SourceLocation loc        = source_map.find_loc(lineNumber, columnNumber);

        if (!loc.is_placeholder) {
            pof = token::Token(loc.line,
                               loc.column,
                               loc.length,
                               loc.column + loc.line,
                               "/*error*/",
                               filePath,
                               "<other>");
        } else {
            return {token::Token(), "", ""};
        }
    } else {
        pof = token::Token(lineNumber, 0, 1, 1, "/*error*/", filePath, "<other>");
    }

    return {pof, message, filePath};
}