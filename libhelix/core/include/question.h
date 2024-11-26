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

#include <exception>
#include <typeinfo>

#include "concepts.h"
#include "config.h"
#include "dtypes.h"
#include "libc.h"
#include "libcxx.h"
#include "print.h"
#include "refs.h"
#include "traits.h"
#include "types.h"

#ifndef __$LIBHELIX_QUESTION__
#define __$LIBHELIX_QUESTION__

H_NAMESPACE_BEGIN
H_STD_NAMESPACE_BEGIN

class Error {
  public:
    Error() = default;
    explicit Error(const string &message)
        : message(message) {
        print("panicked: ", message);
    }

    string message;
};

class NullAccessError : public Error {
  public:
    explicit NullAccessError(const string &message)
        : Error(message) {}
};

class null_t {};
inline constexpr null_t null;

template <class T, typename... Es>
class question;

// template <class T, typename... Es>
//     requires std::traits::is_class_v<T> && (std::traits::is_class_v<Es> && ...)
// class question<T, Es...> : public T {
//   public:
//     /// this is for classes
// };

template <class T, typename... Es>
    requires (std::traits::is_class_v<Es> && ...)
class question<T, Es...> {
  public:
    enum class State : char { Value, Null, Error };

    template <typename... Et>
    union LikelyError {
         LikelyError() {}
        ~LikelyError() {}
         LikelyError(const LikelyError &other)                = default;
         LikelyError &operator=(const LikelyError &other)     = default;
         LikelyError(LikelyError &&other) noexcept            = default;
         LikelyError &operator=(LikelyError &&other) noexcept = default;

        alignas(Et...) array<byte, libcxx::max({sizeof(Et)...})> storage{};
    };

    using ErrorT =
        libcxx::conditional_t<sizeof...(Es) == 0, LikelyError<Error>, LikelyError<Es...>>;

    /// \brief defines the '?' operator to see if the value is null
    [[nodiscard]] constexpr bool $inspect() const { return state == State::Value; }
    constexpr T                 &operator*() {
        switch (state) {
            case State::Value:
                return value;
            case State::Null:
                throw NullAccessError("null access");
            case State::Error:
                throwStoredError();
        }

        throw NullAccessError("memory corruption");
    }

    constexpr operator T &() {  // NOLINT(google-explicit-constructor)
        return operator*();
    }

    // Const version for read-only contexts
    constexpr operator const T &() const {  // NOLINT(google-explicit-constructor)
        return operator*();
    }

    constexpr bool operator==(const null_t & /*unused*/) const { return state == State::Null; }
    constexpr bool operator!=(const null_t & /*unused*/) const { return state != State::Null; }

    constexpr T &$cast(T * /*unused*/) { return operator*(); }

    template <typename E,
              typename = libcxx::enable_if_t<(std::traits::is_same_v<E, Error> ||
                                              (std::traits::is_same_v<E, Es> || ...))>>
    constexpr bool $contains(const E & /* unused */)  // NOLINT(google-explicit-constructor)
    {
        return ((state == State::Error) && error_type_index == type_index<E>());
    }
    template <typename E,
              typename = libcxx::enable_if_t<(std::traits::is_same_v<E, Error> ||
                                              (std::traits::is_same_v<E, Es> || ...))>>
    constexpr bool $contains(E && /* unused */)  // NOLINT(google-explicit-constructor)
    {
        return ((state == State::Error) && error_type_index == type_index<E>());
    }

    constexpr question()
        : state(State::Null) {}

    constexpr question(const T &value)  // NOLINT(google-explicit-constructor)
        : state(State::Value) {
        this->value = value;
    }

    constexpr question(T &&value)  // NOLINT(google-explicit-constructor)
        : state(State::Value) {
        this->value = std::ref::move(value);
    }

    template <typename E,
              typename = libcxx::enable_if_t<(std::traits::is_same_v<E, Error> ||
                                              (std::traits::is_same_v<E, Es> || ...))>>
    constexpr question(const E &err)  // NOLINT(google-explicit-constructor)
        : state(State::Error) {
        new (&error.storage) E(err);
    }

    template <typename E,
              typename = libcxx::enable_if_t<(std::traits::is_same_v<E, Error> ||
                                              (std::traits::is_same_v<E, Es> || ...))>>
    constexpr question(E &&err)  // NOLINT(google-explicit-constructor)
        : state(State::Error)
        , error_type_index(type_index<E>()) {
        new (&error.storage) E(std::forward<E>(err));
    }

    constexpr ~question() {
        if (state == State::Error) {
            destroy_error();
        }
    }

    constexpr question(const null_t & /*unused*/)  // NOLINT(google-explicit-constructor)
        : state(State::Null) {}

    constexpr question(null_t && /*unused*/)  // NOLINT(google-explicit-constructor)
        : state(State::Null) {}

    constexpr question(const question &other) {
        switch (other.state) {
            case State::Value:
                value = other.value;
                break;
            case State::Null:
                break;
            case State::Error:
                error            = other.error;
                error_type_index = other.error_type_index;
                break;
        }

        state = other.state;
    }

    constexpr question(question &&other) noexcept {
        switch (other.state) {
            case State::Value:
                value = std::ref::move(other.value);
                break;
            case State::Null:
                break;
            case State::Error:
                error            = std::ref::move(other.error);
                error_type_index = other.error_type_index;
                break;
        }

        state = other.state;
    }

    constexpr question &operator=(const T &val) {
        this->value = val;
        state       = State::Value;
        return *this;
    }

    constexpr question &operator=(T &&val) {
        this->value = std::ref::move(val);
        state       = State::Value;
        return *this;
    }

    constexpr question &operator=(const null_t & /*unused*/) {
        state = State::Null;
        return *this;
    }

    constexpr question &operator=(null_t && /*unused*/) {
        state = State::Null;
        return *this;
    }

    constexpr question &operator=(const question &other) {
        switch (other.state) {
            case State::Value:
                value = other.value;
                break;
            case State::Null:
                break;
            case State::Error:
                error            = other.error;
                error_type_index = other.error_type_index;
                break;
        }

        state = other.state;
        return *this;
    }

    constexpr question &operator=(question &&other) noexcept {
        switch (other.state) {
            case State::Value:
                value = std::ref::move(other.value);
                break;
            case State::Null:
                break;
            case State::Error:
                error            = std::ref::move(other.error);
                error_type_index = other.error_type_index;
                break;
        }

        state = other.state;
        return *this;
    }

  private:
    T      value;
    ErrorT error;
    State  state            = State::Null;
    size_t error_type_index = 0;

    constexpr void destroy_error() {
        visit_error([&](auto *errPtr) {
            if constexpr (requires { errPtr->~decltype (*errPtr)(); }) {
                errPtr->~decltype (*errPtr)();
            }
        });
    }

    // Helper function to throw the stored error
    constexpr void throwStoredError() const {
        visit_error([&](auto *errPtr) { throw *errPtr; });
    }

    template <typename E>
    static constexpr size_t type_index() {
        return typeid(E).hash_code();
    }

    template <typename F>
    constexpr void visit_error(F &&func) const {
        if constexpr (sizeof...(Es) > 0) {
            // For each type in Es..., check if it matches the stored type
            (..., ([&] {
                 if (error_type_index == type_index<Es>()) {
                     func(reinterpret_cast /* NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) */
                          <const Es *>(&error.storage));
                 }
             }()));

        } else {
            func(reinterpret_cast /* NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) */
                 <const Error *>(&error.storage));
        }
    }
};

H_STD_NAMESPACE_END
H_NAMESPACE_END
#endif