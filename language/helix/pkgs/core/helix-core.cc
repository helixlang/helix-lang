///*--- Helix ---*
///
/// Part of the Helix Project, under the Attribution 4.0 International license (CC BY 4.0). You are
/// allowed to use, modify, redistribute, and create derivative works, even for commercial purposes,
/// provided that you give appropriate credit, and indicate if changes were made. For more
/// information, please visit: https://creativecommons.org/licenses/by/4.0/
///
/// SPDX-License-Identifier: CC-BY-4.0
/// Copyright (c) 2024 (CC BY 4.0)
///
/// \warning: This file is a part of the helix-core library, using this file externally may result
///           in undefined behavior.
///
///*--- Helix ---*

// auto c++ includes for the core of the language
#include <set>
#include <map>
#include <tuple>
#include <array>
#include <limits>
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <variant>
#include <sstream>
#include <utility>
#include <concepts>
#include <optional>
#include <iostream>
#include <stdexcept>
#include <coroutine>
#include <type_traits>

#ifdef __GNUG__
    #include <cxxabi.h>
#endif

/// language primitive types

/// ensure cross-platform compatibility for 128-bit and 256-bit types.
/// gcc/clang support these types, but MSVC may not. We can conditionally include them.

using u8   = unsigned char;
using i8   = signed char;
using u16  = unsigned short;
using i16  = signed short;
using u32  = unsigned int;
using i32  = signed int;
using u64  = unsigned long long;
using i64  = signed long long;

#if !defined(_MSC_VER)
    using u128 = __uint128_t;
    using i128 = __int128_t;
#endif

using f32 = float;
using f64 = double;
using f80 = long double;

using usize = std::size_t;
using isize = std::ptrdiff_t;

using byte = std::byte;
using string = std::string;

template <typename ...Args>
using tuple = std::tuple<Args...>;
template <typename ...Args>
using list = std::vector<Args...>;
template <typename ...Args>
using set = std::set<Args...>;
template <typename ...Args>
using map = std::map<Args...>;

#if __cplusplus < 202002L
static_assert(false, "helix requires c++20 or higher");
#endif

/// \include belongs to the helix standard library.
/// \brief namespace for helix standard library in c++
namespace helix {
/// \include belongs to the helix standard library.
/// \brief namespace for helix standard library
namespace std {
/// \include belongs to the helix standard library.
/// \brief namespace for internal interfaces
namespace __internal_interfaces {
    /// \include belongs to the helix standard library.
    /// \brief concept for converting a type to a string
    ///
    /// This concept checks if a type has a to_string method that returns a string
    ///
    template <typename T>
    concept ToString = requires(T a) {
        { a.to_string() } -> ::std::convertible_to<::string>;
    };

    /// \include belongs to the helix standard library.
    /// \brief concept for converting a type to a ostream
    ///
    /// This concept checks if a type has an ostream operator
    ///
    template <typename T>
    concept OStream = requires(::std::ostream &os, T a) {
        { os << a } -> ::std::convertible_to<::std::ostream &>;
    };

    /// \include belongs to the helix standard library.
    /// \brief concept for converting a type to a string
    ///
    /// This concept checks if a type can be converted to a string
    ///
    template <typename T>
    concept CanConvertToStringForm = ToString<T> || OStream<T>;
}  // namespace __internal_interfaces

/// \include belongs to the helix standard library.
/// \brief convert any type to a string
///
/// This function will try to convert the argument to a string using the following methods:
/// - if the argument has a to_string method, it will use that
/// - if the argument has a ostream operator, it will use that
/// - if the argument is an arithmetic type, it will use std::to_string
/// - if all else fails, it will convert the address of the argument to a string
///
template <typename Expr>
constexpr ::string any_to_string(Expr &&arg) {
    if constexpr (__internal_interfaces::ToString<Expr>) {
        return arg.to_string();
    } else if constexpr (__internal_interfaces::OStream<Expr>) {
        ::std::stringstream ss;
        ss << arg;
        return ss.str();
    } else if constexpr (::std::is_arithmetic_v<Expr>) {
        return ::std::to_string(arg);
    } else {
        ::std::stringstream ss;
    
#       ifdef __GNUG__
            int status;
            char *realname = abi::__cxa_demangle(typeid(arg).name(), 0, 0, &status);
            ss << "[" << realname << " at 0x" << ::std::hex << &arg << "]";
            free(realname);
#       else
            ss << "[" << typeid(arg).name() << " at 0x" << ::std::hex << &arg << "]";
#       endif

        return ss.str();
    }
}

/// \include belongs to the helix standard library.
/// \brief format a string with arguments
///
/// TODO: = is not yet suppoted
///
/// the following calls can happen in helix and becomes the following c++:
///
/// f"hi: {var}"   -> format_string("hi: \{\}", var)
/// f"hi: {var1=}" -> format_string("hi: var1=\{\}", var1)
///
/// f"hi: {(some_expr() + 12)=}" -> format_string("hi: (some_expr() + 12)=\{\}", some_expr())
/// f"hi: {some_expr() + 12}"    -> format_string("hi: \{\}", some_expr() + 12)
///
template <typename... Expr>
constexpr  ::string format_string( ::string base, Expr &&...args) {
    const ::std::array< ::string, sizeof...(args)> exprs_as_string = {
        any_to_string(::std::forward<Expr>(args))...};
    size_t pos = 0;

#pragma unroll
    for (auto &&arg : exprs_as_string) {
        pos = base.find("\\{\\}", pos);

        if (pos == ::string::npos) [[unlikely]] {
            throw ::std::runtime_error(
                "error: [f-stirng engine]: format argument count mismatch, this should not happen, please open a issue on github");
        }

        base.replace(pos, 4, arg);
        pos += arg.size();
    }

    return base;
}
}  // namespace std

class endl {
  public:
    endl &operator=(const endl &) = delete;
    endl &operator=(endl &&)      = delete;
    endl(const endl &end)         = delete;
    endl(endl &&)                 = delete;
    endl()                        = default;
    ~endl()                       = default;

    explicit endl(::string end)
        : end_l(::std::move(end)) {}
    
    explicit endl(const char *end)
        : end_l(end) {}
    
    explicit endl(const char end)
        : end_l(::string(1, end)) {}
    
    friend ::std::ostream &operator<<(::std::ostream &oss, const endl &end) {
        oss << end.end_l;
        return oss;
    }

  private:
    ::string end_l = "\n";
};

template <::std::movable T>
class generator {
  public:
    struct promise_type {
        generator<T> get_return_object() { return generator{Handle::from_promise(*this)}; }
        static ::std::suspend_always initial_suspend() noexcept { return {}; }
        static ::std::suspend_always final_suspend() noexcept { return {}; }
        ::std::suspend_always        yield_value(T value) noexcept {
            current_value = ::std::move(value);
            return {};
        }
        // Disallow co_await in generator coroutines.
        void await_transform() = delete;
        [[noreturn]]
        static void unhandled_exception() {
            throw;
        }

        ::std::optional<T> current_value;
    };

    using Handle = ::std::coroutine_handle<promise_type>;

    explicit generator(const Handle coroutine)
        : m_coroutine{coroutine} {}

    generator() = default;
    ~generator() {
        if (m_coroutine)
            m_coroutine.destroy();
    }

    generator(const generator &)            = delete;
    generator &operator=(const generator &) = delete;

    generator(generator &&other) noexcept
        : m_coroutine{other.m_coroutine} {
        other.m_coroutine = {};
    }
    generator &operator=(generator &&other) noexcept {
        if (this != &other) {
            if (m_coroutine)
                m_coroutine.destroy();
            m_coroutine       = other.m_coroutine;
            other.m_coroutine = {};
        }
        return *this;
    }

    // Range-based for loop support.
    class Iter {
      public:
        void     operator++() { m_coroutine.resume(); }
        const T &operator*() const { return *m_coroutine.promise().current_value; }
        bool     operator==(::std::default_sentinel_t) const {
            return !m_coroutine || m_coroutine.done();
        }

        explicit Iter(const Handle coroutine)
            : m_coroutine{coroutine} {}

      private:
        Handle m_coroutine;
    };

    Iter begin() {
        if (m_coroutine)
            m_coroutine.resume();
        return Iter{m_coroutine};
    }

    ::std::default_sentinel_t end() { return {}; }

  private:
    Handle m_coroutine;
};
}  // namespace helix

template <typename... Args>
inline constexpr void print(Args &&...args) {
    if constexpr (sizeof...(args) == 0) {
        ::std::cout << helix::endl('\n');
        return;
    };
    
    (::std::cout << ... << args);
    
    if constexpr (sizeof...(args) > 0) {
        if constexpr (!::std::is_same_v<::std::remove_cv_t<::std::remove_reference_t<
                                          decltype(::std::get<sizeof...(args) - 1>(
                                              ::std::tuple<Args...>(args...)))>>,
                                      helix::endl>) {
            ::std::cout << helix::endl('\n');
        }
    }
}

#define _new(type, ...) new type(__VA_ARGS__)