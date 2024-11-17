// auto c++ includes for the core of the language
#include <array>
#include <concepts>
#include <coroutine>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#ifdef __GNUG__
#include <cxxabi.h>
#endif

/// language primitive types

/// ensure cross-platform compatibility for 128-bit and 256-bit types.
/// gcc/clang support these types, but MSVC may not. We can conditionally include them.

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

using usize = ::std::size_t;
using isize = ::std::ptrdiff_t;

using byte   = ::std::byte;
using string = ::std::string;

template <typename... Args>
using tuple = ::std::tuple<Args...>;
template <typename... Args>
using list = ::std::vector<Args...>;
template <typename... Args>
using set = ::std::set<Args...>;
template <typename... Args>
using map = ::std::map<Args...>;

#if __cplusplus < 202002L
static_assert(false, "helix requires c++20 or higher");
#endif

/// \include belongs to the helix standard library.
/// \brief namespace for helix standard library in c++
namespace helix {
namespace libcxx::std = ::std;

/// \include belongs to the helix standard library.
/// \brief namespace for helix standard library
namespace std {
    template <class _Tp, _Tp __v>
    struct integral_constant {
        static constexpr const _Tp value = __v;
        typedef _Tp                value_type;
        typedef integral_constant  type;
        constexpr                  operator value_type() const { return value; }
        constexpr value_type       operator()() const { return value; }
    };

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
            { a.to_string() } -> libcxx::convertible_to<::string>;
        };

        /// \include belongs to the helix standard library.
        /// \brief concept for converting a type to a ostream
        ///
        /// This concept checks if a type has an ostream operator
        ///
        template <typename T>
        concept OStream = requires(libcxx::ostream &os, T a) {
            { os << a } -> libcxx::convertible_to<libcxx::ostream &>;
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
            libcxx::stringstream ss;
            ss << arg;
            return ss.str();
        } else if constexpr (libcxx::is_arithmetic_v<Expr>) {
            if constexpr (libcxx::is_floating_point_v<Expr>) {
                helix::libcxx::cout << "floating point\n";
                return libcxx::to_string(arg);
            } else {
                return libcxx::to_string(arg);
            }
        } else {
            libcxx::stringstream ss;

#ifdef __GNUG__
            int   status;
            char *realname = abi::__cxa_demangle(typeid(arg).name(), 0, 0, &status);
            ss << "[" << realname << " at 0x" << libcxx::hex << &arg << "]";
            free(realname);
#else
            ss << "[" << typeid(arg).name() << " at 0x" << libcxx::hex << &arg << "]";
#endif

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
    constexpr ::string format_string(::string base, Expr &&...args) {
        const libcxx::array<::string, sizeof...(args)> exprs_as_string = {
            any_to_string(libcxx::forward<Expr>(args))...};

        size_t pos = 0;

#pragma unroll
        for (auto &&arg : exprs_as_string) {
            pos = base.find("\\{\\}", pos);

            if (pos == ::string::npos) [[unlikely]] {
                throw libcxx::runtime_error(
                    "error: [f-stirng engine]: format argument count mismatch, this should not "
                    "happen, please open a issue on github");
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
        : end_l(libcxx::move(end)) {}

    explicit endl(const char *end)
        : end_l(end) {}

    explicit endl(const char end)
        : end_l(::string(1, end)) {}

    friend libcxx::ostream &operator<<(libcxx::ostream &oss, const endl &end) {
        oss << end.end_l;
        return oss;
    }

  private:
    ::string end_l = "\n";
};

template <libcxx::movable T>
class generator {
  public:
    struct promise_type {
        static libcxx::suspend_always initial_suspend() noexcept { return {}; }
        static libcxx::suspend_always final_suspend() noexcept { return {}; }
        generator<T> get_return_object() { return generator{Handle::from_promise(*this)}; }

        libcxx::suspend_always yield_value(T value) noexcept {
            current_value = libcxx::move(value);
            return {};
        }

        void                     await_transform() = delete;
        [[noreturn]] static void unhandled_exception() { throw; }

        libcxx::optional<T> current_value;
    };

    using Handle = libcxx::coroutine_handle<promise_type>;

    generator()                             = default;
    generator(const generator &)            = delete;
    generator &operator=(const generator &) = delete;
    explicit generator(const Handle coroutine)
        : m_coroutine{coroutine} {}

    generator(generator &&other) noexcept
        : m_coroutine{other.m_coroutine} {
        other.m_coroutine = {};
    }

    generator &operator=(generator &&other) noexcept {
        if (this != &other) {
            if (m_coroutine) {
                m_coroutine.destroy();
            }

            m_coroutine       = other.m_coroutine;
            other.m_coroutine = {};
        }
        return *this;
    }

    ~generator() {
        if (m_coroutine) {
            m_coroutine.destroy();
        }
    }

    class Iter {
      public:
        void     operator++() { m_coroutine.resume(); }
        const T &operator*() const { return *m_coroutine.promise().current_value; }
        bool     operator==(libcxx::default_sentinel_t /*unused*/) const {
            return !m_coroutine || m_coroutine.done();
        }

        explicit Iter(const Handle coroutine)
            : m_coroutine{coroutine} {}

        size_t index() const { return m_coroutine.promise().index; }

      private:
        Handle m_coroutine;
    };

    Iter begin() {
        if (m_coroutine) {
            m_coroutine.resume();
        }

        if (m_iter == nullptr) {
            m_iter = new Iter{m_coroutine};
        }

        return *m_iter;
    }

    libcxx::default_sentinel_t end() { return {}; }

  private:
    Handle m_coroutine;
    Iter  *m_iter = nullptr;
};


/// \include belongs to the helix standard library.
/// \brief function to forward arguments
///
/// \code {.cpp}
/// int main() {
///     int* a = (int*)malloc(sizeof(int) * 10);
///     $finally _([&] { free(a); });
/// }
class $finally {
    public:
        $finally() = default;
        $finally(const $finally &) = delete;
        $finally($finally &&)      = delete;
        $finally &operator=(const $finally &) = delete;
        $finally &operator=($finally &&) = delete;
        ~$finally() {
            if (m_fn) {
                m_fn();
            }
        }
    
        template <typename Fn>
        explicit $finally(Fn &&fn)
            : m_fn{libcxx::forward<Fn>(fn)} {}
    
    private:
        libcxx::function<void()> m_fn;
};
}  // namespace helix

/// next function to advance a generator manually and return the current value
template <helix::libcxx::movable T>
inline T next(helix::generator<T> &gen) {
    auto iter = gen.begin();
    return *iter;
}

template <typename... Args>
inline constexpr void print(Args &&...args) {
    if constexpr (sizeof...(args) == 0) {
        helix::libcxx::cout << helix::endl('\n');
        return;
    };

    (helix::libcxx::cout << ... << helix::std::any_to_string(helix::libcxx::forward<Args>(args)));

    if constexpr (sizeof...(args) > 0) {
        if constexpr (!helix::libcxx::is_same_v<
                          helix::libcxx::remove_cv_t<helix::libcxx::remove_reference_t<
                              decltype(helix::libcxx::get<sizeof...(args) - 1>(
                                  helix::libcxx::tuple<Args...>(args...)))>>,
                          helix::endl>) {
            helix::libcxx::cout << helix::endl('\n');
        }
    }
}

#define _new(type, ...) new type(__VA_ARGS__)

namespace libc {
template <typename T>
using va_array = T[];

template <typename T, size_t N>
using array = T[N];
}  // namespace libc
