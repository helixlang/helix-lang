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
///------------------------------------------------------------------------------------ Helix ---///

#include <cstddef>
#include <array>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "config.h"

#ifndef __$LIBHELIX_DTYPES__
#define __$LIBHELIX_DTYPES__

H_NAMESPACE_BEGIN

using byte   = libcxx::byte;
using string = libcxx::string;

template <typename _Ty, std::size_t _Size>
using array  = libcxx::array<_Ty, _Size>;
template <typename... Args>
using tuple  = libcxx::tuple<Args...>;
template <typename _Ty>
using list   = libcxx::vector<_Ty>;
template <typename... Args>
using set    = libcxx::set<Args...>;
template <typename... Args>
using map    = libcxx::map<Args...>;

using $int = int;

H_NAMESPACE_END
#endif
