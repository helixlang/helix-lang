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

#ifndef __$LIBHELIX_TYPES__
#define __$LIBHELIX_TYPES__

using u8  = unsigned char;
using i8  = signed char;
using u16 = unsigned short;
using i16 = signed short;
using u32 = unsigned int;
using i32 = signed int;
using u64 = unsigned long long;
using i64 = signed long long;

#if !defined(_MSC_VER)
using u128 = __uint128_t;
using i128 = __int128_t;
#endif

using f32 = float;
using f64 = double;
using f80 = long double;

using usize = std::size_t;
using isize = std::ptrdiff_t;

#endif