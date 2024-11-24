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

#include <functional>

#include "config.h"
#include "refs.h"

#ifndef __$LIBHELIX_FINALLY__
#define __$LIBHELIX_FINALLY__

H_NAMESPACE_BEGIN

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
    $finally()                            = default;
    $finally(const $finally &)            = delete;
    $finally($finally &&)                 = delete;
    $finally &operator=(const $finally &) = delete;
    $finally &operator=($finally &&)      = delete;
    ~$finally() {
        if (m_fn) {
            m_fn();
        }
    }

    template <typename Fn>
    explicit $finally(Fn &&fn)
        : m_fn{std::forward<Fn>(fn)} {}

  private:
    libcxx::function<void()> m_fn;
};

H_NAMESPACE_END
#endif