#include "controller/include/tooling/tooling.hh"

#if IS_UNIX

#include <unistd.h>

#include <array>
#include <cstdio>
#include <stdexcept>
#include <string>

CXIRCompiler::ExecResult CXIRCompiler::exec(const std::string &cmd) {
    std::array<char, 128> buffer;
    std::string           result;

    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("popen() failed to initialize command execution.");
    }

    try {
        while (feof(pipe) == 0) {
            if (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
                result += buffer.data();
            }
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }

    // Close the pipe and check the return code
    int rc = pclose(pipe);
    if (rc == 0) {
        return {result, 0};
    }

    return {result, rc};
}

#endif
