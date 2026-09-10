/// --- The Kairo Project ------------------- Sema/DependentResolution.k --- ///
///
///   \brief Resolve DependentType nodes (unresolved generic exprs).
///
///   \detail
///     Decide whether e.g. `N + 1` in generic context is a value-expr or a
///     generic-arg, lowering each to a binop or a generic call. Precedes
///     Monomorphization.
///
///   Part of the Kairo Project, under the Apache License v2.0 with the
///   Kairo Runtime Library Exception.
///
///   See: https://www.kairolang.org/LICENSE.txt
///   SPDX-License-Identifier: Apache-2.0 WITH KAIRO-RUNTIME-EXCEPTION
///   Copyright (c) 2026 Dhruvan Kartik
///
/// ----------------------------------------------------------------- ///

import AST::Node::Base::*;

class DependentResolution {
    // TODO: implement DependentResolution

    // A member through an instance of an IMPORTED template is not answered
    // here: the instance itself is filled from clang (ForeignInstantiate::fill).
}
