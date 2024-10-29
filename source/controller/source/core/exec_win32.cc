#include "controller/include/tooling/tooling.hh"

#if IS_WIN32

#include <windows.h>

#include <array>
#include <memory>
#include <stdexcept>
#include <string>

CXIRCompiler::ExecResult CXIRCompiler::exec(const std::string &cmd) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength              = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle       = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE hReadPipe = nullptr, hWritePipe = nullptr;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        throw std::runtime_error("CreatePipe failed!");
    }

    if (!SetHandleInformation(hWritePipe, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        throw std::runtime_error("SetHandleInformation failed!");
    }

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb         = sizeof(si);
    si.hStdOutput = hWritePipe;
    si.hStdError  = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;

    if (!CreateProcess(nullptr,
                       const_cast<char *>(cmd.c_str()),
                       nullptr,
                       nullptr,
                       TRUE,
                       CREATE_NO_WINDOW,
                       nullptr,
                       nullptr,
                       &si,
                       &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        throw std::runtime_error("CreateProcess failed!");
    }

    CloseHandle(hWritePipe);

    std::array<char, 128> buffer;
    std::string           result;
    DWORD                 bytesRead;

    while (ReadFile(hReadPipe, buffer.data(), buffer.size(), &bytesRead, nullptr) &&
           bytesRead > 0) {
        result.append(buffer.data(), bytesRead);
    }

    CloseHandle(hReadPipe);
    CloseHandle(pi.hThread);

    DWORD exitCode;
    if (GetExitCodeProcess(pi.hProcess, &exitCode) == 0) {
        CloseHandle(pi.hProcess);
        throw std::runtime_error("GetExitCodeProcess failed!");
    }

    CloseHandle(pi.hProcess);
    return {result, static_cast<int>(exitCode)};
}

#endif
