/// --- The Kairo Project ------------------------- Codegen/EmitPlan.k --- ///
///
///   \brief Per emitted TU, work out the declarations a C++ header would
///          have provided, and the order they have to appear in.
///
///   \detail
///     IMPORTS.md §0: imports are erased at name resolution. After N and
///     ChainBinding every use site holds a `*Decl`, and the decl knows its
///     own home through the DeclContext parent chain. Nothing here reads an
///     `ImportDecl`. What a C++ TU still needs, and what a `#include` used
///     to supply, is a DECLARATION of everything this TU names but does not
///     define. This pass computes that set from the resolved links alone.
///
///     Three steps, IMPORTS.md §6.1-§6.3:
///
///       roots    every foreign decl this TU actually names. Walked off
///                `NamedIdentExpr::resolved_decl`, every `ChainExpr::Step`,
///                every entry of a deferred `candidate_cell`, and every
///                canonical `RecordType`. Foreign = the decl's own fid is
///                not this TU's fid.
///       closure  what those roots in turn require, to a fixed point. Two
///                edge strengths: `Fwd` (a `class X;` suffices) and
///                `Complete` (the body-less definition must precede).
///                Invariant 11: a class enters whole or not at all.
///       tiers    tier 0 every type as a forward declaration, which breaks
///                every pointer cycle up front; tier 1 the definitions,
///                topologically sorted on Complete edges ONLY; tier 2 the
///                function declarations, any order.
///
///     A Complete cycle is not this pass's error to report. TypeCycleCheck
///     owns it, in sema, where a diagnostic is possible; by the time the
///     plan runs a cycle means sema let one through, so the sort asserts.
///     That is IMPORTS.md §6.3, and it is why item 10 came before item 11.
///
///     The output is ORDERED and DETERMINISTIC. Within a tier the tie-break
///     is (source fid, source offset), so the same input produces the same
///     plan on every machine and a diff of two plans means a real change.
///
///     What this pass does NOT do: emit. It hands `entries` to an emitter
///     (IMPORTS.md §6.6: one `InterfaceEmitter`, two callers -- EmitPlan for
///     foreign decls, EmitCXXHeader for this module's `pub` decls). Keeping
///     the plan separate from the emission is what lets the header and the
///     per-TU preamble share every line of the hard part.
///
///   Import law: CodegenContext only, like every other file under Codegen/.
///   No clang headers.
///
///   Phase: CODEGEN (produces CodegenPhase::Planned)
///
///   Part of the Kairo Project, under the Apache License v2.0 with the
///   Kairo Runtime Library Exception.
///
///   See: https://www.kairolang.org/LICENSE.txt
///   SPDX-License-Identifier: Apache-2.0 WITH KAIRO-RUNTIME-EXCEPTION
///   Copyright (c) 2026 Dhruvan Kartik
///
/// ------------------------------------------------------------------------ ///

import Codegen::CodegenContext::*;
import AST::Context::ASTContext::*;
import AST::Context::DeclContext::*;
import AST::Context::TranslationUnit::*;
import AST::Context::SymbolTable::*;
import AST::ASTSupport::*;
import AST::Node::Base::*;
import AST::Node::Program::*;
import AST::Node::Types::*;
import AST::Node::CanonicalNodes::*;
import AST::Node::Expressions::*;
import AST::Node::PrimaryExpressions::*;
import AST::Node::Declarations::*;
import AST::Node::Casting::*;
import AST::Kind::DeclKind::*;
import AST::Kind::TypeKind::*;
import AST::Kind::ExprKind::*;
import AST::Visitor::RecursiveASTVisitor::*;
import Location::SourceManager::*;

/// \enum PlanNeed
/// \brief How much of a decl the plan owes the C++ TU.
///
/// The two strengths are not a preference, they are what C++ requires: a
/// pointer member is satisfied by `class X;`, a by-value member is not.
/// Complete subsumes Fwd; the closure raises a decl from Fwd to Complete and
/// never lowers it.
enum PlanNeed derives u8 {
    Fwd = 0,   ///< `class X;` is enough.
    Complete,  ///< the body-less definition must precede.
}

/// \enum PlanTier
/// \brief Which block of the emitted preamble a decl lands in.
enum PlanTier derives u8 {
    /// Every type in the set, as a forward declaration. Emitted first so
    /// pointer cycles never constrain tier 1's sort.
    FwdDecls = 0,
    /// Type definitions, topologically sorted on Complete edges.
    Definitions,
    /// Function declarations. Any order.
    Functions,
}

/// \class PlanEntry
/// \brief One line of the plan: a decl, how much of it, and where.
class PlanEntry {
    pub var decl: *Decl    = &null;
    pub var need: PlanNeed = PlanNeed::Fwd;
    pub var tier: PlanTier = PlanTier::FwdDecls;
    /// The decl's own fid and offset. The tier's tie-break, kept on the
    /// entry so the sort does not re-resolve a location per comparison.
    pub var fid:  u32      = 0;
    pub var off:  usize    = 0;

    fn PlanEntry(self) { }

    fn PlanEntry(self, d: *Decl, need: PlanNeed, tier: PlanTier, fid: u32, off: usize) {
        self.decl = d;
        self.need = need;
        self.tier = tier;
        self.fid  = fid;
        self.off  = off;
    }
}

/// \class EmitPlanProbe
/// \brief "Does this decl's layout reach \p target by value?", asked once
///        per tier-1 edge query.
///
/// Split out of EmitPlan so the layout spine is written once and both the
/// base-list and field-list forms of the question share it. Holds a borrowed
/// pointer only and lives for one query.
class EmitPlanProbe {
    priv var _target: obs::<Decl>;

    fn EmitPlanProbe(self, target: *Decl) {
        self._target = target;
    }

    fn bases(self, list: *FrozenList::<InheritEdge>) -> bool {
        for var i: usize = 0; i < list->size(); ++i {
            var e = list->get(i);
            if e == &null { continue; }
            if self._reaches(e->base) { return true; }
        }
        return false;
    }

    fn fields(self, list: *FrozenList::<*FieldDecl>) -> bool {
        for var i: usize = 0; i < list->size(); ++i {
            var f = *(list->get(i));
            if f == &null { continue; }
            if self._reaches(f->type_) { return true; }
        }
        return false;
    }

    /// The by-value spine, and only it. A pointer, nullable, vector, map or
    /// set stops the walk: tier 0 already forward-declared whatever is
    /// behind it, so it constrains nothing here.
    priv fn _reaches(self, t: *Type) -> bool {
        if t == &null { return false; }
        var c = t->canonical;
        if c == &null { c = t; }

        switch c->kind {
            case TypeKind::RecordType {
                var r = cast::<RecordType>(c);
                if r->decl == self._target { return true; }
                for var i: usize = 0; i < r->args.size(); ++i {
                    if self._reaches(*(r->args.get(i))) { return true; }
                }
                return false;
            } case TypeKind::TupleType {
                var tp = cast::<TupleType>(c);
                for var i: usize = 0; i < tp->elements.size(); ++i {
                    if self._reaches(*(tp->elements.get(i))) { return true; }
                }
                return false;
            } case TypeKind::FixedArrayType {
                return self._reaches(cast::<FixedArrayType>(c)->element);
            } default {
                return false;
            }
        }
        return false;
    }
}

/// \class EmitPlan
/// \brief The plan for one TU. Construct, run, read `entries`.
///
///     var plan = EmitPlan(&cg);
///     plan.run();
///     for e in plan.entries { ... }
///
class EmitPlan derives RecursiveASTVisitor::<EmitPlan> {
    priv var _cg: obs::<CodegenContext>;

    /// This TU's fid. A decl from anywhere else is foreign and belongs in
    /// the plan; a decl from here is emitted by the normal walk.
    priv var _home: u32 = 0;

    /// The working set, in discovery order. `_need` is parallel to it: one
    /// PlanNeed per decl, raised in place by the closure.
    priv var _set:  vec::<*Decl>;
    priv var _need: vec::<PlanNeed>;

    /// Closure frontier: indices into `_set` not yet expanded. A decl is
    /// re-queued when its need is RAISED, because Complete pulls in strictly
    /// more than Fwd did.
    priv var _queue: vec::<usize>;

    /// The finished plan. Empty until run() returns.
    pub var entries: vec::<PlanEntry>;

    fn EmitPlan(self, cg: *CodegenContext) {
        self._cg = cg;
    }

    // --- Entry point ---------------------------------------------------------

    fn run(self) {
        var tu = self._cg->tu;
        if tu == &null { return; }
        if tu->program == &null { return; }
        self._home = tu->fid;

        self._collect_roots();
        self._close();
        self._order();
    }

    // --- Dump ----------------------------------------------------------------

    /// `--print-emit-plan`. One line per entry, in plan order, so a diff of
    /// two dumps is a diff of two plans. Deliberately not the C++ text: this
    /// is what the plan DECIDED, and the emitter is a separate question.
    fn dump(self, path: string) {
        std::print(f"===== EMIT PLAN fid {self._home}: {path} =====");
        if self.entries.size() == 0 {
            std::print("  (empty: this TU names nothing foreign)");
            return;
        }
        var tier = u8(255);
        for var i: usize = 0; i < self.entries.size(); ++i {
            var e = &(self.entries[i]);
            if static_cast::<u8>(e->tier) != tier {
                tier = static_cast::<u8>(e->tier);
                std::print(f"  -- {self._tier_name(e->tier)} --");
            }
            var nm = self._qual_name(e->decl, e->fid);
            var kd = self._kind_name(e->decl);
            std::print(f"    {kd} {nm}   [{self._need_name(e->need)}] fid {e->fid} @ {e->off}");
        }
    }

    priv fn _tier_name(self, t: PlanTier) -> string {
        switch t {
            case PlanTier::FwdDecls    { return "tier 0: forward declarations"; }
            case PlanTier::Definitions { return "tier 1: definitions"; }
            case PlanTier::Functions   { return "tier 2: function declarations"; }
        }
        return "tier ?";
    }

    priv fn _need_name(self, n: PlanNeed) -> string {
        if n == PlanNeed::Complete { return "complete"; }
        return "fwd";
    }

    priv fn _kind_name(self, d: *Decl) -> string {
        switch d->kind {
            case DeclKind::ClassDecl     { return "class"; }
            case DeclKind::StructDecl    { return "struct"; }
            case DeclKind::UnionDecl     { return "union"; }
            case DeclKind::EnumDecl      { return "enum"; }
            case DeclKind::InterfaceDecl { return "interface"; }
            case DeclKind::FunctionDecl  { return "fn"; }
            default { return "decl"; }
        }
        return "decl";
    }

    /// `mod::Inner::Name`, built from the semantic DeclContext chain --
    /// invariant 9: a decl's namespace is derived from where it lives, never
    /// stored per use site.
    priv fn _qual_name(self, d: *Decl, fid: u32) -> string {
        var out = self._plain_name(d, fid);
        var dc = reinterpret_cast::<*DeclContext>(d->semantic_dc);
        while dc != &null {
            var owner = dc->dc_owner;
            if owner == &null { break; }
            if owner->kind == DeclKind::ModuleDecl {
                var seg = self._imm(cast::<ModuleDecl>(owner)->name.imm, fid);
                out = seg + "::" + out;
            }
            dc = dc->dc_parent;
        }
        return out;
    }

    priv fn _plain_name(self, d: *Decl, fid: u32) -> string {
        switch d->kind {
            case DeclKind::ClassDecl     { return self._imm(cast::<ClassDecl>(d)->name.imm, fid);     }
            case DeclKind::StructDecl    { return self._imm(cast::<StructDecl>(d)->name.imm, fid);    }
            case DeclKind::UnionDecl     { return self._imm(cast::<UnionDecl>(d)->name.imm, fid);     }
            case DeclKind::EnumDecl      { return self._imm(cast::<EnumDecl>(d)->name.imm, fid);      }
            case DeclKind::InterfaceDecl { return self._imm(cast::<InterfaceDecl>(d)->name.imm, fid); }
            case DeclKind::FunctionDecl  { return self._imm(cast::<FunctionDecl>(d)->name.imm, fid);  }
            default { return "<decl>"; }
        }
        return "<decl>";
    }

    /// A decl's name imm is an index into ITS OWN TU's table (RESOLUTION.md
    /// hard invariant #2), so rendering a foreign decl means asking that
    /// fid's table, never this one's.
    priv fn _imm(self, imm: u32, fid: u32) -> string {
        if imm == u32(-1) { return "<?>"; }
        var tcm = self._cg->ctx->tcm;
        if tcm == &null { return "<?>"; }
        var tbl = tcm->get_imm_table_for_fid(fid);
        if tbl == &null { return "<?>"; }
        return tbl->get_ident(imm);
    }

    // --- §6.1 Root set -------------------------------------------------------
    //
    // One RAV over this TU. Every resolved link is a candidate; `_admit`
    // filters to foreign records and functions. The walk is over the whole
    // tree rather than a curated subset because "what this TU names" is
    // exactly "what survived name resolution as a resolved_decl", and any
    // curation is a place for a name to go missing silently.

    priv fn _collect_roots(self) {
        var tu = self._cg->tu;
        var items = &(tu->program->items);
        for var i: usize = 0; i < items->size(); ++i {
            var item = *(items->get(i));
            switch item.kind {
                case TopLevelKind::Decl { self.dispatch_decl(item.as_decl()); }
                case TopLevelKind::Stmt { self.dispatch_stmt(item.as_stmt()); }
                case TopLevelKind::Expr { self.dispatch_expr(item.as_expr()); }
                default { }
            }
        }
    }

    /// RAV family hook: every expression in the TU.
    fn visit_expr(self, e: *Expr) -> bool {
        if e == &null { return true; }
        switch e->kind {
            case ExprKind::NamedIdentExpr {
                var n = cast::<NamedIdentExpr>(e);
                self._admit(n->resolved_decl, PlanNeed::Fwd);
                self._admit_cell(n->candidate_cell);
            } case ExprKind::ChainExpr {
                var c = cast::<ChainExpr>(e);
                for var i: usize = 0; i < c->steps.size(); ++i {
                    var s = c->steps.get(i);
                    if s == &null { continue; }
                    self._admit(s->resolved_decl, PlanNeed::Fwd);
                    self._admit_cell(s->candidate_cell);
                }
            } default {
                break;
            }
        }
        return true;
    }

    /// RAV family hook: every type node in the TU, at Fwd.
    ///
    /// Naming a type is not the same as needing its layout, and RAV descends
    /// THROUGH indirections: the `bar::Deep` inside `*bar::Deep` is its own
    /// type node and reaches this hook exactly like a by-value one. So this
    /// hook can only ever say "declare it". Layout strength comes from
    /// `visit_decl`, which sees the DECLARED type whole and hands it to
    /// `_admit_type`, where a pointer weakens the edge on the way down.
    fn visit_type(self, t: *Type) -> bool {
        if t == &null { return true; }
        var c = t->canonical;
        if c == &null { return true; }
        if c->kind != TypeKind::RecordType { return true; }
        self._admit(cast::<RecordType>(c)->decl, PlanNeed::Fwd);
        return true;
    }

    /// RAV family hook: the declarations of this TU. A declared type is a
    /// LAYOUT position -- a variable, a field, a parameter, a return type --
    /// so it enters at Complete and `_admit_type` weakens it wherever the
    /// written type puts an indirection in the way.
    fn visit_decl(self, d: *Decl) -> bool {
        if d == &null { return true; }
        switch d->kind {
            case DeclKind::VariableDecl {
                self._admit_type(cast::<VariableDecl>(d)->type_, PlanNeed::Complete);
            } case DeclKind::FieldDecl {
                self._admit_type(cast::<FieldDecl>(d)->type_, PlanNeed::Complete);
            } case DeclKind::ParamDecl {
                self._admit_type(cast::<ParamDecl>(d)->type_, PlanNeed::Complete);
            } case DeclKind::FunctionDecl {
                // The return type is a layout position at every call site in
                // this TU; the parameters arrive through their own ParamDecls.
                self._admit_type(cast::<FunctionDecl>(d)->return_type, PlanNeed::Complete);
            } case DeclKind::TypeAliasDecl {
                self._admit_type(cast::<TypeAliasDecl>(d)->aliased, PlanNeed::Complete);
            } default {
                break;
            }
        }
        return true;
    }

    priv fn _admit_cell(self, cell: *OverloadCell) {
        if cell == &null { return; }
        for var i: usize = 0; i < cell->size(); ++i {
            self._admit(cell->get(i), PlanNeed::Fwd);
        }
    }

    // --- The working set -----------------------------------------------------

    /// Add \p d to the plan at \p need, or raise an existing entry to it.
    /// Everything funnels through here: this is the one place that decides
    /// what is foreign and what is plannable.
    priv fn _admit(self, d: *Decl, need: PlanNeed) {
        if d == &null { return; }
        var c = self._canon(d);
        if c == &null { return; }

        if !self._plannable(c) { return; }
        // Declared by its header (the PCH / #include); Kairo never re-emits a
        // decl it imported from C++ (IMPORTS.md §5).
        if self._is_imported(c) { return; }
        if self._fid_of(c) == self._home { return; }   // ours; emitted normally

        var at = self._index_of(c);
        if at == usize(-1) {
            self._set.push_back(c);
            self._need.push_back(need);
            self._queue.push_back(self._set.size() - 1);
            return;
        }
        // Raise, never lower. A raise re-queues: Complete pulls in strictly
        // more than Fwd did, and the decl may already have been expanded.
        if static_cast::<u8>(need) > static_cast::<u8>(self._need[at]) {
            self._need[at] = need;
            self._queue.push_back(at);
        }
    }

    /// Follow a type down to the records it needs, at the strength the
    /// SHAPE dictates: a pointee or a nullable inner needs only a forward
    /// declaration, everything on the by-value spine needs the definition.
    priv fn _admit_type(self, t: *Type, need: PlanNeed) {
        if t == &null { return; }
        var c = t->canonical;
        if c == &null { c = t; }

        switch c->kind {
            case TypeKind::RecordType {
                var r = cast::<RecordType>(c);
                self._admit(r->decl, need);
                // A generic argument rides at the same strength as the
                // instance: `Vec<Foo>` by value needs Foo's definition, a
                // `*Vec<Foo>` needs neither.
                for var i: usize = 0; i < r->args.size(); ++i {
                    self._admit_type(*(r->args.get(i)), need);
                }
            } case TypeKind::PointerType {
                self._admit_type(cast::<PointerType>(c)->pointee, PlanNeed::Fwd);
            } case TypeKind::NullableType {
                self._admit_type(cast::<NullableType>(c)->inner, PlanNeed::Fwd);
            } case TypeKind::VectorType {
                self._admit_type(cast::<VectorType>(c)->element, PlanNeed::Fwd);
            } case TypeKind::SetType {
                self._admit_type(cast::<SetType>(c)->element, PlanNeed::Fwd);
            } case TypeKind::MapType {
                var m = cast::<MapType>(c);
                self._admit_type(m->key,   PlanNeed::Fwd);
                self._admit_type(m->value, PlanNeed::Fwd);
            } case TypeKind::TupleType {
                var tp = cast::<TupleType>(c);
                for var i: usize = 0; i < tp->elements.size(); ++i {
                    self._admit_type(*(tp->elements.get(i)), need);
                }
            } case TypeKind::FixedArrayType {
                self._admit_type(cast::<FixedArrayType>(c)->element, need);
            } case TypeKind::FunctionPointerType {
                // A function POINTER needs neither side complete.
                var f = cast::<FunctionPointerType>(c);
                for var i: usize = 0; i < f->params.size(); ++i {
                    self._admit_type(*(f->params.get(i)), PlanNeed::Fwd);
                }
                self._admit_type(f->return_type, PlanNeed::Fwd);
            } default {
                break; // builtin, generic param, error: nothing to declare
            }
        }
    }

    // --- §6.2 Closure --------------------------------------------------------
    //
    // Fixed point over the frontier. Every decl in the set is expanded at
    // the need it currently carries; a raise re-queues it, so the loop ends
    // only when nothing changed. The edge table is IMPORTS.md §6.2.

    priv fn _close(self) {
        while self._queue.size() > 0 {
            var at = self._queue[self._queue.size() - 1];
            self._queue.pop_back();
            self._expand(self._set[at], self._need[at]);
        }
    }

    priv fn _expand(self, d: *Decl, need: PlanNeed) {
        switch d->kind {
            case DeclKind::FunctionDecl {
                // A declaration only. Parameters and the return type are
                // named, not laid out, so Fwd is enough for both.
                var n = cast::<FunctionDecl>(d);
                for var i: usize = 0; i < n->params.size(); ++i {
                    var p = *(n->params.get(i));
                    if p == &null { continue; }
                    self._admit_type(p->type_, PlanNeed::Fwd);
                }
                self._admit_type(n->return_type, PlanNeed::Fwd);
            } case DeclKind::ClassDecl {
                if need != PlanNeed::Complete { return; }
                var n = cast::<ClassDecl>(d);
                self._bases(&(n->derives_list));
                self._bases(&(n->implements));
                self._fields(&(n->fields));
                self._methods(&(n->methods));
            } case DeclKind::StructDecl {
                if need != PlanNeed::Complete { return; }
                var n = cast::<StructDecl>(d);
                self._bases(&(n->implements));
                self._fields(&(n->fields));
                self._methods(&(n->methods));
            } case DeclKind::UnionDecl {
                if need != PlanNeed::Complete { return; }
                var n = cast::<UnionDecl>(d);
                self._fields(&(n->fields));
                self._methods(&(n->methods));
            } case DeclKind::InterfaceDecl {
                if need != PlanNeed::Complete { return; }
                var n = cast::<InterfaceDecl>(d);
                self._bases(&(n->derives_list));
                self._methods(&(n->methods));
            } case DeclKind::EnumDecl {
                // `underlying` is a builtin, so an enum closes over its ADT
                // payloads and nothing else.
                if need != PlanNeed::Complete { return; }
                var n = cast::<EnumDecl>(d);
                for var i: usize = 0; i < n->variants.size(); ++i {
                    var v = *(n->variants.get(i));
                    if v == &null { continue; }
                    self._fields(&(v->payload));
                }
            } case DeclKind::TypeAliasDecl {
                // An alias is transparent: whoever needed the alias needs
                // the target at the same strength.
                self._admit_type(cast::<TypeAliasDecl>(d)->aliased, need);
            } default {
                break;
            }
        }
    }

    /// A base contributes its whole layout, so a base edge is Complete
    /// whatever brought the derived type in.
    priv fn _bases(self, bases: *FrozenList::<InheritEdge>) {
        for var i: usize = 0; i < bases->size(); ++i {
            var e = bases->get(i);
            if e == &null { continue; }
            self._admit_type(e->base, PlanNeed::Complete);
        }
    }

    /// A value field needs its type's definition; a pointer or nullable
    /// field does not. `_admit_type` splits those two on the way down, so
    /// the field edge starts at Complete and weakens where the shape says.
    priv fn _fields(self, fields: *FrozenList::<*FieldDecl>) {
        for var i: usize = 0; i < fields->size(); ++i {
            var f = *(fields->get(i));
            if f == &null { continue; }
            self._admit_type(f->type_, PlanNeed::Complete);
        }
    }

    /// Invariant 11: a class enters the interface WHOLE -- every member
    /// declared. So a method's signature types come along, at Fwd: a
    /// signature names its types, it does not lay them out.
    priv fn _methods(self, methods: *FrozenList::<*FunctionDecl>) {
        for var i: usize = 0; i < methods->size(); ++i {
            var m = *(methods->get(i));
            if m == &null { continue; }
            for var j: usize = 0; j < m->params.size(); ++j {
                var p = *(m->params.get(j));
                if p == &null { continue; }
                self._admit_type(p->type_, PlanNeed::Fwd);
            }
            self._admit_type(m->return_type, PlanNeed::Fwd);
        }
    }

    // --- §6.3 Ordering -------------------------------------------------------

    priv fn _order(self) {
        // Tier 0: every TYPE in the set gets a forward declaration, whatever
        // its need. Emitting all of them up front is what makes tier 1's
        // sort a DAG problem rather than a cycle problem: a pointer edge
        // between two definitions is already satisfied by this tier.
        var t0: vec::<PlanEntry>;
        for var i: usize = 0; i < self._set.size(); ++i {
            var d = self._set[i];
            if !self._is_type_decl(d) { continue; }
            t0.push_back(self._entry(d, PlanNeed::Fwd, PlanTier::FwdDecls));
        }
        self._sort(&t0);
        for var i: usize = 0; i < t0.size(); ++i { self.entries.push_back(t0[i]); }

        // Tier 1: definitions, topologically sorted on Complete edges only.
        var t1: vec::<PlanEntry>;
        for var i: usize = 0; i < self._set.size(); ++i {
            var d = self._set[i];
            if !self._is_type_decl(d) { continue; }
            if self._need[i] != PlanNeed::Complete { continue; }
            t1.push_back(self._entry(d, PlanNeed::Complete, PlanTier::Definitions));
        }
        self._sort(&t1);
        self._toposort(&t1);
        for var i: usize = 0; i < t1.size(); ++i { self.entries.push_back(t1[i]); }

        // Tier 2: function declarations. Any order, so the tie-break IS the
        // order.
        var t2: vec::<PlanEntry>;
        for var i: usize = 0; i < self._set.size(); ++i {
            var d = self._set[i];
            if d->kind != DeclKind::FunctionDecl { continue; }
            t2.push_back(self._entry(d, PlanNeed::Fwd, PlanTier::Functions));
        }
        self._sort(&t2);
        for var i: usize = 0; i < t2.size(); ++i { self.entries.push_back(t2[i]); }
    }

    priv fn _entry(self, d: *Decl, need: PlanNeed, tier: PlanTier) -> PlanEntry {
        return PlanEntry(d, need, tier, self._fid_of(d), self._off_of(d));
    }

    /// (fid, offset) ascending. Insertion sort: a tier holds the foreign
    /// decls of one TU, which is tens of entries, and an insertion sort is
    /// both stable and the whole of its own correctness argument.
    priv fn _sort(self, v: *vec::<PlanEntry>) {
        for var i: usize = 1; i < v->size(); ++i {
            var j = i;
            while j > 0 {
                var a = &((*v)[j - 1]);
                var b = &((*v)[j]);
                var swap = false;
                if a->fid > b->fid { swap = true; }
                else if (a->fid == b->fid) && (a->off > b->off) { swap = true; }
                if !swap { break; }
                var tmp = (*v)[j - 1];
                (*v)[j - 1] = (*v)[j];
                (*v)[j] = tmp;
                j -= 1;
            }
        }
    }

    /// Stable topological sort of tier 1 on Complete edges. Repeatedly take
    /// the first entry all of whose Complete predecessors are already
    /// placed; the input is already in tie-break order, so "first" makes the
    /// result deterministic.
    ///
    /// A pass with nothing takeable means a Complete cycle, which sema was
    /// supposed to have rejected (TypeCycleCheck, IMPORTS.md §6.3). Assert
    /// rather than diagnose: codegen has no way to report a source error.
    priv fn _toposort(self, v: *vec::<PlanEntry>) {
        var n = v->size();
        if n < 2 { return; }

        var out:   vec::<PlanEntry>;
        var taken: vec::<bool>;
        for var i: usize = 0; i < n; ++i { taken.push_back(false); }

        for var round: usize = 0; round < n; ++round {
            var picked = usize(-1);
            for var i: usize = 0; i < n; ++i {
                if taken[i] { continue; }
                var ready = true;
                for var j: usize = 0; j < n; ++j {
                    if taken[j] { continue; }
                    if j == i { continue; }
                    // j must precede i?
                    if self._needs_complete(((*v)[i]).decl, ((*v)[j]).decl) {
                        ready = false;
                        break;
                    }
                }
                if ready { picked = i; break; }
            }
            assert((picked != usize(-1))
                && r"EmitPlan: Complete-edge cycle in tier 1; TypeCycleCheck should have rejected this in sema");
            taken[picked] = true;
            out.push_back((*v)[picked]);
        }

        for var i: usize = 0; i < n; ++i { (*v)[i] = out[i]; }
    }

    /// True when \p who's definition requires \p dep's definition first: a
    /// base, a by-value field, a by-value tuple/array element, an alias
    /// target. Recomputed per query rather than materialized as an adjacency
    /// list -- tier 1 is small, and one edge rule with one caller is easier
    /// to keep honest than a table built somewhere else.
    priv fn _needs_complete(self, who: *Decl, dep: *Decl) -> bool {
        if who == &null { return false; }
        if dep == &null { return false; }
        var probe = EmitPlanProbe(dep);
        switch who->kind {
            case DeclKind::ClassDecl {
                var n = cast::<ClassDecl>(who);
                if probe.bases(&(n->derives_list)) { return true; }
                if probe.bases(&(n->implements))   { return true; }
                return probe.fields(&(n->fields));
            } case DeclKind::StructDecl {
                var n = cast::<StructDecl>(who);
                if probe.bases(&(n->implements)) { return true; }
                return probe.fields(&(n->fields));
            } case DeclKind::UnionDecl {
                return probe.fields(&(cast::<UnionDecl>(who)->fields));
            } case DeclKind::InterfaceDecl {
                return probe.bases(&(cast::<InterfaceDecl>(who)->derives_list));
            } case DeclKind::EnumDecl {
                var n = cast::<EnumDecl>(who);
                for var i: usize = 0; i < n->variants.size(); ++i {
                    var vr = *(n->variants.get(i));
                    if vr == &null { continue; }
                    if probe.fields(&(vr->payload)) { return true; }
                }
                return false;
            } default {
                return false;
            }
        }
        return false;
    }

    // --- Predicates and small helpers ----------------------------------------

    /// The decl kinds a plan can name. A module is a namespace, not a decl
    /// to declare; an import is erased (invariant 8); a variable's home TU
    /// defines it and an `extern` for it is a linker concern, not an
    /// interface one.
    priv fn _is_imported(self, d: *Decl) -> bool {
        if isa::<FunctionDecl>(d)  { return cast::<FunctionDecl>(d)->is_imported();  }
        if isa::<ClassDecl>(d)     { return cast::<ClassDecl>(d)->is_imported();     }
        if isa::<StructDecl>(d)    { return cast::<StructDecl>(d)->is_imported();    }
        if isa::<UnionDecl>(d)     { return cast::<UnionDecl>(d)->is_imported();     }
        if isa::<EnumDecl>(d)      { return cast::<EnumDecl>(d)->is_imported();      }
        if isa::<InterfaceDecl>(d) { return cast::<InterfaceDecl>(d)->is_imported(); }
        return false;
    }

    priv fn _plannable(self, d: *Decl) -> bool {
        return self._is_type_decl(d) || (d->kind == DeclKind::FunctionDecl);
    }

    priv fn _is_type_decl(self, d: *Decl) -> bool {
        return (d->kind == DeclKind::ClassDecl)
            || (d->kind == DeclKind::StructDecl)
            || (d->kind == DeclKind::UnionDecl)
            || (d->kind == DeclKind::EnumDecl)
            || (d->kind == DeclKind::InterfaceDecl);
    }

    /// The entity's representative decl. `class Foo;` and `class Foo { }`
    /// are one entity and must be one plan entry, which is exactly what the
    /// redecl canonical link answers.
    priv fn _canon(self, d: *Decl) -> *Decl {
        switch d->kind {
            case DeclKind::ClassDecl {
                return static_cast::<*Decl>(cast::<ClassDecl>(d)->canonical_decl());
            } case DeclKind::StructDecl {
                return static_cast::<*Decl>(cast::<StructDecl>(d)->canonical_decl());
            } case DeclKind::UnionDecl {
                return static_cast::<*Decl>(cast::<UnionDecl>(d)->canonical_decl());
            } case DeclKind::EnumDecl {
                return static_cast::<*Decl>(cast::<EnumDecl>(d)->canonical_decl());
            } case DeclKind::InterfaceDecl {
                return static_cast::<*Decl>(cast::<InterfaceDecl>(d)->canonical_decl());
            } case DeclKind::FunctionDecl {
                return static_cast::<*Decl>(cast::<FunctionDecl>(d)->canonical_decl());
            } default {
                return d;
            }
        }
        return d;
    }

    priv fn _index_of(self, d: *Decl) -> usize {
        for var i: usize = 0; i < self._set.size(); ++i {
            if self._set[i] == d { return i; }
        }
        return usize(-1);
    }

    priv fn _fid_of(self, d: *Decl) -> u32 {
        var sm = self._cg->ctx->sm;
        if sm == &null { return self._home; }
        if !d->range.start.is_valid() { return self._home; }
        return sm->get_fid(d->range.start);
    }

    priv fn _off_of(self, d: *Decl) -> usize {
        var sm = self._cg->ctx->sm;
        if sm == &null { return 0; }
        if !d->range.start.is_valid() { return 0; }
        return sm->get_offset(d->range.start);
    }
}
