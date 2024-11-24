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

#include "config.h"

#ifndef __$LIBHELIX_REFS__
#define __$LIBHELIX_REFS__

H_NAMESPACE_BEGIN
H_STD_NAMESPACE_BEGIN
namespace ref {
template <typename T>
struct remove_ref {
    using t = T;
};

template <typename T>
struct remove_ref<T &> {
    using t = T;
};

template <typename T>
struct remove_ref<T &&> {
    using t = T;
};

template <typename T>
struct remove_cv {
    using t = T;
};

template <typename T>
struct remove_cv<const T> {
    using t = T;
};

template <typename T>
struct remove_cv<volatile T> {
    using t = T;
};

template <typename T>
struct remove_cv<const volatile T> {
    using t = T;
};

template <typename T>
struct remove_cvref {
    using t = typename remove_cv<typename remove_ref<T>::t>::t;
};

template <typename T>
using remove_ref_t = typename remove_ref<T>::t;

template <typename T>
using remove_cv_t = typename remove_cv<T>::t;

template <typename T>
using remove_cvref_t = typename remove_cvref<T>::t;

template <class _Ty>
constexpr remove_ref_t<_Ty>&& move(_Ty&& _Arg) noexcept {
    return static_cast<remove_ref_t<_Ty>&&>(_Arg);
}
}  // end namespace ref

// namespace std

template <typename T>
T &&forward(typename ref::remove_ref_t<T> &t) noexcept {
    return static_cast<T &&>(t);
}

template <typename T>
T &&forward(typename ref::remove_ref_t<T> &&t) noexcept {
    return static_cast<T &&>(t);
}

H_STD_NAMESPACE_END
H_NAMESPACE_END
#endif