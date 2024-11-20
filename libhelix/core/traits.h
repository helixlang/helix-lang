#ifndef HELIX_TRAITS_H
#define HELIX_TRAITS_H

#include "__traits.h"

namespace helix::std {
    template <class _Tp, _Tp __v>
    struct integral_constant {
        static constexpr const _Tp value = __v;
        typedef _Tp                value_type;
        typedef integral_constant  type;
        constexpr                  operator value_type() const { return value; }
        constexpr value_type       operator()() const { return value; }
    };

    template <class _Tp>
    _Tp&& __declval(int);
    template <class _Tp>
    _Tp __declval(long);

    template <class _Tp>
    _LIBCPP_HIDE_FROM_ABI decltype(__declval<_Tp>(0)) declval() _NOEXCEPT {
    static_assert(!__is_same(_Tp, _Tp),
                    "std::declval can only be used in an unevaluated context. "
                    "It's likely that your current usage is trying to extract a value from the function.");
    }

    template <class _From, class _To>
    concept convertible_to = __is_convertible(_From, _To) && requires { static_cast<_To>(declval<_From>()); };

    template <class _Tp, class _Up>
    concept same_as = integral_constant<bool, __is_same(_Tp, _Up)>::value &&
                      integral_constant<bool, __is_same(_Up, _Tp)>::value;

    template <class _Tp, class _Up>
    inline constexpr bool is_same_v = __is_same(_Tp, _Up);

    template <class _Bp, class _Dp>
    struct _LIBCPP_TEMPLATE_VIS is_base_of
        : public integral_constant<bool, __is_base_of(_Bp, _Dp)> {};

    template <class _Bp, class _Dp>
    inline constexpr bool is_base_of_v = __is_base_of(_Bp, _Dp);
}

#endif  // HELIX_TRAITS_H
