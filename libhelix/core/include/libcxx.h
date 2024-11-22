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

#ifndef __$LIBHELIX_LIBCXX__
#define __$LIBHELIX_LIBCXX__

#include "config.h"
#include "refs.h"

H_NAMESPACE_BEGIN
H_STD_NAMESPACE_BEGIN

template <typename _Tp, typename... _Ty>
constexpr _Tp *_new(_Ty &&...t) {
    return new _Tp(std::forward<_Ty>(t)...);
}

H_STD_NAMESPACE_END
H_NAMESPACE_END
#endif