
///--- Helix Core: Concepts ---------------------------------------------------------------------///
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
///----------------------------------------------------------------------------------------------///
///                                                                                              ///
///   This file defines type-related concepts and utilities for use in Helix runtime and tools.  ///
///   These concepts enable type constraints and introspection at compile time.                  ///
///                                                                                              ///
///------------------------------------------------------------------------------------ Helix ---///

#ifndef HELIX_CONCEPTS_H
#define HELIX_CONCEPTS_H

#include <type_traits>
#include <string>
#include <concepts>

BEGIN_LIBHELIX_NAMESPACE

template <class T>
concept ToString = requires(T a) {
    { a.to_string() } -> std::convertible_to<std::string>;
};

template <class T>
concept OStream = requires(std::ostream& os, T a) {
    { os << a } -> std::convertible_to<std::ostream&>;
};

template <class T>
concept CanConvertToStringForm = ToString<T> || OStream<T>;

}  // namespace std
}  // namespace helix

#endif // HELIX_CONCEPTS_H
