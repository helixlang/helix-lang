// -*- C++ -*-
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

#ifndef __COMPILER_HH__
#define __COMPILER_HH__

#include <vector>

namespace clang {
using namespace std;

struct Arguments {
    vector<string> args;  //> additional arguments to pass
    vector<string> libs;  //> lib dirs, such as for libcxx
    vector<string> incs;  //> include dirs
    vector<string> link;  //> link dirs
};

class Compiler {
  private:
    Arguments args;

  public:
    Compiler() = default;
    explicit Compiler(Arguments args);
};
}  // namespace clang

#endif  // __COMPILER_HH__