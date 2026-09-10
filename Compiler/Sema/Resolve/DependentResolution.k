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

    /// `T::Item` through \p inst, an instance of an IMPORTED template
    /// (MemberLookup::filled_by_clang). Clang owns that instantiation, so the
    /// answer is clang's: complete the ClassTemplateSpecializationDecl behind
    /// the primary's `clang_decl` with clang::Sema::RequireCompleteType, under
    /// the owning HeaderSet's sema_lock(), and map the member it finds back
    /// through the ForeignRegistry.
    ///
    /// STUB: null, which every caller already reads as "still dependent".
    fn member_of_foreign_instance(self, inst: *Decl, name: string) -> *Decl {
        return &null;
    }
}
