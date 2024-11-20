#ifndef HELIX__TRAITS_H
#define HELIX__TRAITS_H

/// \include belongs to the helix standard library.
/// \brief namespace for internal interfaces
namespace helix::std::__internal_interfaces {
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

#endif  // HELIX__TRAITS_H
