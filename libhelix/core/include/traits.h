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

#ifndef __$LIBHELIX_TRAITS__
#define __$LIBHELIX_TRAITS__

H_NAMESPACE_BEGIN
H_STD_NAMESPACE_BEGIN
namespace traits {

template <class _Tp, _Tp __v>
struct integral_constant {
    using value_type = _Tp;
    using type       = integral_constant;

    static constexpr const _Tp value = __v;

    constexpr            operator value_type() const { return value; }
    constexpr value_type operator()() const { return value; }
};

/// portable bindings for declval
template <class _Tp>
_Tp &&__declval(int);
template <class _Tp>
_Tp __declval(long);

template <class _Tp>
_LIBCPP_HIDE_FROM_ABI decltype(__declval<_Tp>(0)) declval() _NOEXCEPT {
    static_assert(
        !__is_same(_Tp, _Tp),
        "std::declval can only be used in an unevaluated context. "
        "It's likely that your current usage is trying to extract a value from the function.");
}

template <class _Up, class _Tp>
constexpr bool is_convertible_v = __is_convertible_to(_Up, _Tp);

template <class _Up, class _Tp>
concept convertible_to =
#if !defined(__clang__) && !defined(__EDG__)
    is_convertible_v<_Up, _Tp>
#else
    __is_convertible_to(_Up, _Tp)
#endif
    && requires { static_cast<_Tp>(declval<_Up>()); };

#ifdef __clang__

template <class _Tp, class _Up>
constexpr bool same_as_v = __is_same(_Tp, _Up);

template <class _Tp, class _Up>
concept same_as = integral_constant<bool, same_as_v<_Tp, _Up>>::value ||
                  integral_constant<bool, same_as_v<_Up, _Tp>>::value;

#else

template <class, class>
constexpr bool same_as_v = false;

template <class _Tp>
constexpr bool same_as_v<_Tp, _Tp> = true;

template <class _Tp, class _Up>
concept same_as = same_as_v<_Tp, _Up>;

#endif

/// clang binding for same_as_v
template <class _Tp, class _Up>
inline constexpr bool is_same_v = same_as_v<_Tp, _Up>;

template <class _Bp, class _Dp>
struct _LIBCPP_TEMPLATE_VIS is_derived_of : public integral_constant<bool, __is_base_of(_Bp, _Dp)> {};

template <class _Bp, class _Dp>
inline constexpr bool is_derived_of_v = __is_base_of(_Bp, _Dp);

template <class _Tp>
struct is_class : integral_constant<bool, __is_class(_Tp)> {}; // determine whether _Tp is a class

template <class _Tp>
constexpr bool is_class_v = __is_class(_Tp);

namespace _const::utils {
    template <class>
    constexpr bool is_const_v = false;

    template <class _Tp>
    constexpr bool is_const_v<const _Tp> = true;
}  // namespace _const::utils

template <class _Tp>
struct _LIBCPP_TEMPLATE_VIS is_const : integral_constant<bool, _const::utils::is_const_v<_Tp>> {};

}  // namespace traits
H_STD_NAMESPACE_END
H_NAMESPACE_END
#endif