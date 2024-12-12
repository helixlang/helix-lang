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

#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64)

#include <windows.h>

#include <array>
#include <stdexcept>
#include <string>

CXIRCompiler::ExecResult CXIRCompiler::exec(const std::string &cmd) {
    SECURITY_ATTRIBUTES sa         = {sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE              hReadPipe  = nullptr;
    HANDLE              hWritePipe = nullptr;

    if (CreatePipe(&hReadPipe, &hWritePipe, &sa, 0) == 0) {
        throw std::runtime_error("CreatePipe failed! Error: " + std::to_string(GetLastError()));
    }

    if (SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0) == 0) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        throw std::runtime_error("SetHandleInformation failed! Error: " +
                                 std::to_string(GetLastError()));
    }

    PROCESS_INFORMATION pi = {};
    STARTUPINFO         si = {};
    si.cb                  = sizeof(si);
    si.hStdOutput          = hWritePipe;
    si.hStdError           = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;

    if (!CreateProcess(nullptr,
                       const_cast<char *>(cmd.c_str()), // NOLINT
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
        throw std::runtime_error("CreateProcess failed! Error: " + std::to_string(GetLastError()));
    }

    CloseHandle(hWritePipe);

    std::string           result;
    std::array<char, 128> buffer{};

    DWORD bytesRead = 0;

    std::thread readerThread([&]() {
        while (true) {
            if (ReadFile(hReadPipe, buffer.data(), buffer.size(), &bytesRead, nullptr) == 0) {
                if (GetLastError() == ERROR_BROKEN_PIPE) {
                    break;
                }
                throw std::runtime_error("ReadFile failed! Error: " +
                                         std::to_string(GetLastError()));
            }
            result.append(buffer.data(), bytesRead);
        }
    });

    DWORD waitResult = WaitForSingleObject(pi.hProcess, 10000);  // 10 sec timeout
    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 1);
        readerThread.join();
        CloseHandle(hReadPipe);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        throw std::runtime_error("Process timed out!");
    }

    readerThread.join();

    DWORD exitCode = 0;
    if (GetExitCodeProcess(pi.hProcess, &exitCode) == 0) {
        CloseHandle(hReadPipe);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        throw std::runtime_error("GetExitCodeProcess failed! Error: " +
                                 std::to_string(GetLastError()));
    }

    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return {result, static_cast<int>(exitCode)};
}

#endif
