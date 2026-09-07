/// --- The Kairo Project --------------------------- Sema/NameLookup.k --- ///
///
///   \brief THE shared lookup core. NameResolution (N) and TypeResolution
///          (T) both stand on these four primitives; neither pass owns
///          private lookup logic. A component on SemaContext, not a pass.
///
///   Import direction: this file imports AST only. SemaContext imports
///   this; passes reach it via sc->lookup. It must never import
///   SemaContext or any pass (cycle rule, see SemaContext.k).
///
///   Phase: LOWER (component; used from N onward)
///
///   Part of the Kairo Project, under the Apache License v2.0 with the
///   Kairo Runtime Library Exception.
///
///   See: https://www.kairolang.org/LICENSE.txt
///   SPDX-License-Identifier: Apache-2.0 WITH KAIRO-RUNTIME-EXCEPTION
///   Copyright (c) 2026 Dhruvan Kartik
///
/// ------------------------------------------------------------------------ ///

import AST::Context::ASTContext::*;
import AST::Context::TranslationUnit::*;
import AST::Context::DeclContext::*;
import AST::Context::SymbolTable::*;
import AST::Context::ImportOverlay::*;
import AST::Node::Base::*;
import AST::Node::Declarations::*;
import AST::Node::Casting::*;

/// \class SemaLookupResult
/// \brief One lookup's outcome: the frozen cell + where it was found.
///        Ephemeral by law (see file header).
class SemaLookupResult {
    pub var cell:        *OverloadCell = &null;  ///< null = not found.
    pub var found_in:    *DeclContext  = &null;  ///< scope that answered.
    pub var via_overlay: bool          = false;  ///< true if the TU import overlay answered.

    static fn not_found() -> SemaLookupResult {
        var r = SemaLookupResult{};
        return r;
    }

    fn found(self) -> bool {
        return (self.cell != &null) && (self.cell->size() > 0);
    }

    /// Exactly one candidate the common case, bind directly.
    fn single(self) -> bool {
        return self.found() && self.cell->is_single();
    }

    fn overloaded(self) -> bool {
        return self.found() && self.cell->is_overloaded();
    }

    fn first(self) -> *Decl {
        if !self.found() { return &null; }
        return self.cell->get(0);
    }
}

/// \class NameLookup
/// \brief The four lookup primitives over frozen dc_symbols + overlay.
///
///        1. in_context   : one scope, no walk.
///        2. unqualified  : DC parent chain, then TU import_overlay, miss.
///        3. context_of   : *Decl -> its owned *DeclContext (or null)
///                          the step that makes qualified lookup "the AST
///                          is the trie": a child scope is reached THROUGH
///                          its decl, there is no parallel tree.
///        4. qualified_step: resolve one `::` segment through a prior
///                          segment's decl. Callers loop this; per-segment
///                          resolved_decl slots on ChainType/ChainExpr are
///                          written by the PASS, not by this component.
///
///        NOT here by design: body-local scope stacks (N(b) owns its
///        lexical stack; locals are excluded from symbol tables), member
///        lookup through extensions (needs conformance folding sema
///        builds it on qualified_step later), and ADL (own algorithm,
///        arrives with operator resolution).
class NameLookup {
    pub var tu: obs::<TranslationUnit>;

    fn NameLookup(self) { }

    /// Wire to the TU under resolution. SemaContext calls this once.
    fn init(self, tu: *TranslationUnit) {
        self.tu = tu;
    }

    // --- Primitive 1: single scope --------------------------------------------

    fn in_context(self, dc: *DeclContext, name: DeclName) -> SemaLookupResult {
        return self.in_context(dc, name, "");
    }

    /// Look one name up in ONE scope, whichever TU that scope lives in.
    ///
    /// A DeclContext's table is keyed by ITS OWN TU's imms (hard invariant
    /// #2). Probing a foreign scope with THIS TU's imm index is not a miss --
    /// it is a silent WRONG answer whenever the two interners happen to agree
    /// on an index, which for two files of a few names each they routinely
    /// do. `spelling` is the escape hatch, exactly as in
    /// MemberLookup::in_scope: for a foreign scope the name is re-interned
    /// into that TU's imm space and probed there.
    ///
    /// The no-spelling overload above stays for same-TU callers. A caller
    /// that can reach a foreign scope -- anything walking `::` -- must pass
    /// the spelling.
    fn in_context(self, dc: *DeclContext, name: DeclName, spelling: string) -> SemaLookupResult {
        if dc == &null { return SemaLookupResult::not_found(); }
        var cell = self._probe(dc, name, spelling);
        if cell == &null { return SemaLookupResult::not_found(); }
        var r = SemaLookupResult{};
        r.cell     = cell;
        r.found_in = dc;
        return r;
    }

    /// The imm-translation half of in_context. Only IDENTIFIER payloads are
    /// per-TU: operator and ctor keys are built from TokenKinds, which are
    /// stable across the build.
    priv fn _probe(self, dc: *DeclContext, name: DeclName, spelling: string) -> *OverloadCell {
        if name.kind != DeclNameKind::Identifier { return dc->lookup_symbol(name); }
        if spelling.size() == 0                  { return dc->lookup_symbol(name); }
        var other = NameLookup::tu_of(dc);
        if other == &null       { return dc->lookup_symbol(name); }
        if other == self.tu     { return dc->lookup_symbol(name); }
        if self.tu == &null     { return dc->lookup_symbol(name); }
        var ctx = self.tu->ctx;
        if ctx == &null         { return &null; }
        if ctx->tcm == &null    { return &null; }
        var imms = ctx->tcm->get_imm_table_for_fid(other->fid);
        if imms == &null        { return &null; }
        return SymbolTable::lookup_foreign(dc->dc_symbols, imms, spelling);
    }

    // --- Primitive 2: unqualified ---------------------------------------------

    /// RESOLUTION.md order: this DC's table, then the DC parent chain,
    /// then import_overlay at TU level, then miss. Executable scopes have
    /// no tables (dc_symbols null), so the walk skips them for free
    /// body locals are the caller's lexical stack, checked BEFORE calling
    /// this.
    fn unqualified(self, from: *DeclContext, name: DeclName) -> SemaLookupResult {
        var dc = from;
        while dc != &null {
            var r = self.in_context(dc, name);
            if r.found() { return r; }
            dc = dc->dc_parent;
        }

        // Overlay is LAST and imm-keyed by THIS TU's imms (RESOLUTION.md #2,
        // hard invariant #2). Only plain-identifier names are folded into it;
        // operator keys are compared cross-TU directly and ctors are
        // context-relative, so neither routes through the overlay.
        if (self.tu != &null) && (self.tu->import_overlay != &null) && name.is_ident() {
            var cell = self.tu->import_overlay->lookup_ident(name.payload);
            if cell != &null {
                var r = SemaLookupResult{};
                r.cell        = cell;
                r.found_in    = static_cast::<*DeclContext>(self.tu);
                r.via_overlay = true;
                return r;
            }
        }

        return SemaLookupResult::not_found();
    }

    // --- Primitive 3: decl -> owned scope -------------------------------------

    /// The scope a decl OWNS (its body), or null for non-scope-owning
    /// decls. Type aliases return null on purpose: alias expansion is
    /// TypeResolution's job; qualified lookup through an alias must go
    /// through T's expansion, not silently through here.
    /// The TranslationUnit a scope ultimately lives in, or null.
    static fn tu_of(dc: *DeclContext) -> *TranslationUnit {
        var c = dc;
        while c != &null {
            if c->dc_kind == DeclContextKind::TranslationUnit {
                return reinterpret_cast::<*TranslationUnit>(c);
            }
            c = c->dc_parent;
        }
        return &null;
    }

    /// The ASTContext a decl's NAMES live in, or null when it has no scope to
    /// walk up from.
    ///
    /// Names are interned PER FILE, so spelling a decl reached through an
    /// import with the importing TU's context yields the interner's fallback
    /// text ("$FALLBACK-MISSING-INTERN$ ...") rather than the name. Anything
    /// that renders a decl -- a diagnostic, a dump -- has to ask the decl
    /// where its names live first.
    static fn ctx_of(d: *Decl) -> *ASTContext {
        if d == &null { return &null; }
        var dc = NameLookup::context_of(d);
        if dc == &null {
            if d->semantic_dc != &null      { dc = reinterpret_cast::<*DeclContext>(d->semantic_dc); }
            else if d->lexical_dc != &null  { dc = reinterpret_cast::<*DeclContext>(d->lexical_dc); }
        }
        var tu = NameLookup::tu_of(dc);
        if tu == &null { return &null; }
        return tu->ctx;
    }

    static fn context_of(d: *Decl) -> *DeclContext {
        if d == &null { return &null; }
        if isa::<ClassDecl>(d)     { return static_cast::<*DeclContext>(cast::<ClassDecl>(d));     }
        if isa::<StructDecl>(d)    { return static_cast::<*DeclContext>(cast::<StructDecl>(d));    }
        if isa::<EnumDecl>(d)      { return static_cast::<*DeclContext>(cast::<EnumDecl>(d));      }
        if isa::<UnionDecl>(d)     { return static_cast::<*DeclContext>(cast::<UnionDecl>(d));     }
        if isa::<InterfaceDecl>(d) { return static_cast::<*DeclContext>(cast::<InterfaceDecl>(d)); }
        if isa::<ModuleDecl>(d)    { return static_cast::<*DeclContext>(cast::<ModuleDecl>(d));    }
        if isa::<ExtensionDecl>(d) { return static_cast::<*DeclContext>(cast::<ExtensionDecl>(d)); }
        return &null;
    }

    // --- Primitive 4: one qualified segment -----------------------------------

    /// Resolve `<prev>::name` where \p prev is the decl the previous
    /// segment bound to. Redecl-chain note: the definition's context is
    /// reached through whichever link the previous segment bound; N(a)
    /// links chains before N(b) binds, so canonical/most-recent choice is
    /// the caller's policy, not this primitive's.
    fn qualified_step(self, prev: *Decl, name: DeclName) -> SemaLookupResult {
        var dc = NameLookup::context_of(prev);
        if dc == &null { return SemaLookupResult::not_found(); }
        return self.in_context(dc, name);
    }
}
