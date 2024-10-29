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

    auto pof = token::Token(lineNumber, 0, 1, columnNumber, "/*error*/", filePath, "<other>");

    return {pof, message, filePath};
}

CXIRCompiler::ErrorPOFNormalized CXIRCompiler::parse_gcc_err(std::string gcc_out) {
    return parse_clang_err(gcc_out);
}
CXIRCompiler::ErrorPOFNormalized CXIRCompiler::parse_msvc_err(std::string msvc_out) {
    return parse_clang_err(msvc_out);
}