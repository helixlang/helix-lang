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

#ifndef __$LIBHELIX_CONFIG__
#define __$LIBHELIX_CONFIG__

namespace std {} // namespace std blank to allow for aliasing the std namespace

#ifndef _LIBCPP_NODEBUG
#define _LIBCPP_NODEBUG
#endif

#ifndef _LIBCPP_HIDE_FROM_ABI
#define _LIBCPP_HIDE_FROM_ABI
#endif

#ifndef _LIBHELIX_HIDE_FROM_ABI
#define _LIBHELIX_HIDE_FROM_ABI
#endif

#ifndef _LIBCPP_TEMPLATE_VIS
#define _LIBCPP_TEMPLATE_VIS
#endif

#ifndef _NOEXCEPT
#define _NOEXCEPT noexcept
#endif

namespace helix {
    namespace libcxx = ::std;
}

#define H_NAMESPACE_BEGIN namespace helix {
#define H_NAMESPACE_END }
#define H_STD_NAMESPACE_BEGIN namespace std {
#define H_STD_NAMESPACE_END }

#endif