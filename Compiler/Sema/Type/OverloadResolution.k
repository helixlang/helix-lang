/// --- The Kairo Project ---------------- Sema/Type/OverloadResolution.k --- ///
///
///   \brief Pick one function from a candidate set for a call, given the
///          argument types. A component; ExprTyper calls it, nothing else.
///
///   \detail
///     Viability: arity after defaults and packs, every positional and named
///     argument lands on exactly one parameter, and every landed argument
///     reaches its parameter at rank Exact or Converted. A generic candidate
///     is viable when deduction succeeds and binds every non-pack parameter.
///
///     Ranking (lattice §7): a candidate's rank is the WORST of its argument
///     ranks. Two ranks, no ordering inside rank 2. The single tiebreak, and
///     it is not a conversion ordering: among Exact winners a non-generic
///     candidate beats a generic one, otherwise `fn f(i32)` next to
///     `fn <T> f(T)` is ambiguous on every call. Everything else that ties is
///     ambiguous and errors, even when one conversion looks "narrower".
///
///     Untyped literals rank Exact against any parameter they fit (§5: the
///     literal TAKES the type, it is not converted). After selection the
///     typer types the literal with the parameter type, so the literal's own
///     fit diagnostic still fires at the literal.
///
///     Diagnostics owned here: no viable candidate, ambiguous call, a name
///     that is not callable. The candidate notes are capped so a 40-overload
///     set does not print 40 lines.
///
///   Part of the Kairo Project, under the Apache License v2.0 with the
///   Kairo Runtime Library Exception.
///
///   See: https://www.kairolang.org/LICENSE.txt
///   SPDX-License-Identifier: Apache-2.0 WITH KAIRO-RUNTIME-EXCEPTION
///   Copyright (c) 2026 Dhruvan Kartik
///
/// ------------------------------------------------------------------------ ///

import Sema::SemaContext::*;
import Sema::Resolve::NameLookup::*;
import Sema::Type::TypeUtil::*;
import Sema::Type::Conversion::*;
import Sema::Type::ArgumentDeduction::*;
import AST::ASTSupport::*;
import AST::Node::Base::*;
import AST::Node::Types::*;
import AST::Node::CanonicalNodes::*;
import AST::Node::Declarations::*;
import AST::Node::Casting::*;
import Location::SourceRange::*;
import Diagnostic::Registry::*;

/// \enum LitKind
/// \brief An argument that is an untyped literal carries its shape so the
///        resolver can rank it without a type.
enum LitKind derives u8 {
    None = 0,
    Int,      ///< lo/hi/big/negative meaningful.
    Float,    ///< fits any float.
    Null,     ///< `null`: fits any nullable.
    NullPtr,  ///< `&null`: fits any pointer or nullable pointer.
}

class ArgInfo {
    pub var expr:       *Expr = &null;
    /// The node as it sits in `CallExpr::args`. Differs from `expr` only for
    /// a named argument, where the parser wraps the value (see
    /// CallTyping::arg_info); everything but the dump wants `expr`.
    pub var carrier:    *Expr = &null;
    pub var ty:         *Type = &null;      ///< canonical, null for an untyped literal / unknown.
    pub var range:      SourceRange;
    pub var name:       string = "";        ///< named argument spelling; "" for positional.
    pub var name_range: SourceRange;
    pub var lit:        LitKind = LitKind::None;
    pub var lit_lo:     u64  = 0;
    pub var lit_hi:     u64  = 0;
    pub var lit_big:    bool = false;
    pub var lit_neg:    bool = false;
    pub var dependent:  bool = false;       ///< type unknowable here; poisons nothing.
    fn ArgInfo(self) { }
}

enum OvlOutcome derives u8 {
    Selected = 0,
    Ambiguous,
    NoViable,
    Dependent,   ///< an argument is dependent: decide at M2, no diagnostic.
    Empty,       ///< nothing callable under that name.
}

class Candidate {
    pub var f:            *FunctionDecl = &null;
    pub var viable:       bool  = true;
    pub var rank:         u8    = 0;        ///< 0 Exact, 1 Converted.
    pub var generic:      bool  = false;
    pub var reason:       string = "";
    pub var arg_to_param: vec::<usize>;     ///< per arg: param index, or usize(-1) for pack tail.
    pub var ded:          Deduction;
    fn Candidate(self) { }
}

class OvlResult {
    pub var outcome:      OvlOutcome = OvlOutcome::Empty;
    pub var decl:         *FunctionDecl = &null;
    pub var arg_to_param: vec::<usize>;
    pub var ded:          Deduction;
    /// The type the call was made THROUGH, stripped of nullable and pointer:
    /// `obj.m(x)`'s `obj`. Filled by the caller after resolution, alongside
    /// `ded.bind_from_record`, because it answers a question deduction cannot:
    /// an interface member's `Self` is the receiver, and through a bound
    /// `T impl I` the receiver is `T` rather than any record. Null when the
    /// call had no receiver.
    pub var receiver:     *Type = &null;
    fn OvlResult(self) { }
}

class OverloadResolution {
    pub var sc:   *SemaContext;
    pub var conv: *Conversion;
    pub var tu:   TypeUtil;

    fn OverloadResolution(self) { self.sc = &null; self.conv = &null; }
    fn OverloadResolution(self, sc: *SemaContext, conv: *Conversion) {
        self.sc   = sc;
        self.conv = conv;
        self.tu   = TypeUtil(sc);
    }

    /// \param cands   the decls under the name; non-functions are skipped
    ///                (a mixed cell was N(a)'s error already).
    /// \param has_receiver  `obj.m(...)`: the `self` param is bound to the
    ///                receiver and not matched against args. `Type::m(obj)`
    ///                passes false and `self` is an ordinary first param.
    /// \param receiver  the type `obj` has, already stripped of nullable and
    ///                pointer. Not a duplicate of has_receiver: an interface
    ///                member's `Self` IS this type, and viability has to see
    ///                that -- `fn merge(self, other: Self)` reached through a
    ///                `T impl Drawable` takes a T, so ranking the argument
    ///                against `Drawable` would reject a correct call. Null
    ///                when there is no receiver, or when the caller has none
    ///                to offer.
    fn resolve(self, cands: *vec::<*Decl>, args: *vec::<ArgInfo>, has_receiver: bool,
               at: SourceRange, name: string, receiver: *Type = &null) -> OvlResult {
        var out = OvlResult{};
        out.receiver = receiver;

        for var i: usize = 0; i < args->size(); ++i {
            if (*args)[i].dependent { out.outcome = OvlOutcome::Dependent; return out; }
        }

        var cs = vec::<Candidate>();
        for var i: usize = 0; i < cands->size(); ++i {
            var f = dyn_cast::<FunctionDecl>((*cands)[i]);
            if f == &null { continue; }
            if f->is_poisoned() { continue; }
            if NameLookup::is_specialization(static_cast::<*Decl>(f)) { continue; }   // OverloadResolution pairs later.
            var seen = false;
            for var j: usize = 0; j < cs.size(); ++j {
                if NameLookup::representative(static_cast::<*Decl>(cs[j].f))
                == NameLookup::representative(static_cast::<*Decl>(f)) { seen = true; break; }
            }
            if seen { continue; }   // fwd + def are one entity (RedeclMerge).
            var c = Candidate{};
            c.f       = f;
            c.generic = f->generic_params.size() > 0;
            c.ded     = Deduction(self.sc, self.conv);
            self._judge(&c, args, has_receiver, receiver);
            cs.push_back(c);
        }

        if cs.size() == 0 {
            self.sc->diag->report(DiagID::SC001E, at.start,
                f"'{name}' is not callable"
            ).first(at).emit();
            out.outcome = OvlOutcome::Empty;
            return out;
        }

        // best rank among viable
        var best: u8 = 255;
        for var i: usize = 0; i < cs.size(); ++i {
            if cs[i].viable && (cs[i].rank < best) { best = cs[i].rank; }
        }
        if best == 255 {
            self._report_no_viable(&cs, at, name);
            out.outcome = OvlOutcome::NoViable;
            return out;
        }

        var winners = vec::<usize>();
        for var i: usize = 0; i < cs.size(); ++i {
            if cs[i].viable && (cs[i].rank == best) { winners.push_back(i); }
        }
        // the one tiebreak: non-generic beats generic.
        if winners.size() > 1 {
            var nongen = vec::<usize>();
            for var i: usize = 0; i < winners.size(); ++i {
                if !cs[winners[i]].generic { nongen.push_back(winners[i]); }
            }
            if nongen.size() > 0 { winners = nongen; }
        }

        if winners.size() > 1 {
            self._report_ambiguous(&cs, &winners, at, name);
            out.outcome = OvlOutcome::Ambiguous;
            return out;
        }

        var w = &(cs[winners[0]]);
        out.outcome      = OvlOutcome::Selected;
        out.decl         = w->f;
        out.arg_to_param = w->arg_to_param;
        out.ded          = w->ded;
        return out;
    }

    // === one candidate ======================================================

    priv fn _judge(self, c: *Candidate, args: *vec::<ArgInfo>, has_receiver: bool,
                   receiver: *Type) {
        var f = c->f;

        // parameter slots, skipping the receiver when it is bound elsewhere.
        var params = vec::<*ParamDecl>();
        for var i: usize = 0; i < f->params.size(); ++i {
            var p = f->params[i];
            if p == &null { continue; }
            if p->is_self && has_receiver { continue; }
            params.push_back(p);
        }
        var nparams = params.size();
        var pack_at = usize(-1);
        if f->is_variadic && (nparams > 0) { pack_at = nparams - 1; }

        var filled = vec::<bool>();
        for var i: usize = 0; i < nparams; ++i { filled.push_back(false); }
        for var i: usize = 0; i < args->size(); ++i { c->arg_to_param.push_back(usize(-1)); }

        // 1. place every argument.
        var pos: usize = 0;
        for var i: usize = 0; i < args->size(); ++i {
            var a = &((*args)[i]);
            if a->name.size() > 0 {
                var found = usize(-1);
                for var j: usize = 0; j < nparams; ++j {
                    if self.tu.param_name(params[j]) == a->name { found = j; break; }
                }
                if found == usize(-1) { c->viable = false; c->reason = f"no parameter named '{a->name}\x27"; return; }
                if filled[found]      { c->viable = false; c->reason = f"parameter '{a->name}' given twice"; return; }
                filled[found] = true;
                c->arg_to_param[i] = found;
                continue;
            }
            if (pack_at != usize(-1)) && (pos >= pack_at) {
                c->arg_to_param[i] = usize(-1);   // pack tail
                filled[pack_at] = true;
                continue;
            }
            if pos >= nparams {
                c->viable = false;
                c->reason = f"expects {nparams} argument(s), {args->size()} given";
                return;
            }
            while (pos < nparams) && filled[pos] { pos += 1; }
            if pos >= nparams {
                c->viable = false;
                c->reason = f"expects {nparams} argument(s), {args->size()} given";
                return;
            }
            filled[pos] = true;
            c->arg_to_param[i] = pos;
            pos += 1;
        }

        // 2. missing parameters need defaults.
        for var j: usize = 0; j < nparams; ++j {
            if filled[j] { continue; }
            if j == pack_at { continue; }
            if (params[j]->default_val != &null) || params[j]->has_default { continue; }
            var pn = self.tu.param_name(params[j]);
            c->viable = false;
            c->reason = f"missing argument for parameter '{pn}\x27";
            return;
        }

        // 3. rank every placed argument.
        for var i: usize = 0; i < args->size(); ++i {
            var a  = &((*args)[i]);
            var pi = c->arg_to_param[i];
            var p: *ParamDecl = (params[pi]) if (pi != usize(-1)) else (params[pack_at]);
            var pt: *Type = &null;
            if p->type_ != &null { pt = p->type_->canonical; }
            if (p->type_ != &null) && p->type_->is_poisoned() { c->viable = false; c->reason = "parameter type is invalid"; return; }
            pt = c->ded.through_receiver(c->f, receiver, pt);   // `other: Self` is `other: <receiver>`.

            if pt == &null {
                // dependent-but-uncanonical parameter (`[T; N]`, alias app):
                // cannot decide here; accept at Converted and let M2 judge.
                if (p->type_ != &null) && (p->type_->is_dependent() || p->type_->is_instantiation_dependent()) {
                    if c->rank < 1 { c->rank = 1; }
                    continue;
                }
                c->viable = false; c->reason = "parameter type is unresolved"; return;
            }

            var r = self._rank_arg(c, a, pt, pi == usize(-1));
            if r == ImplicitRank::None {
                var pn = self.tu.param_name(p);
                var got = (self.tu.str(a->ty)) if (a->ty != &null) else ("<literal>");
                c->viable = false;
                c->reason = f"cannot convert argument of type '{got}' to parameter '{pn}' of type '{self.tu.str(pt)}\x27";
                return;
            }
            if (r == ImplicitRank::Converted) && (c->rank < 1) { c->rank = 1; }
        }

        // 4. a generic candidate must have deduced every param.
        if c->generic && (!c->ded.is_complete(static_cast::<*Decl>(f))) {
            c->viable = false;
            c->reason = "could not deduce every generic parameter from the arguments";
            return;
        }
    }

    priv fn _rank_arg(self, c: *Candidate, a: *ArgInfo, pt: *Type, is_pack: bool) -> ImplicitRank {
        // Pack element: the param's canonical is the pack's generic param
        // (VariadicPackType shares it). Bind nothing -- a pack is one slot
        // with many types, and the registry does not model that yet -- accept.
        if is_pack {
            if pt->is_dependent() { return ImplicitRank::Converted; }
            if a->lit != LitKind::None { return self._rank_literal(a, pt); }
            return self.conv->implicit(a->ty, pt);
        }

        if a->lit != LitKind::None {
            if pt->is_dependent() {
                // `fn <T> f(x: T)` with `f(10)`: T := the literal's default.
                var def = self._literal_default(a);
                if def == &null { return ImplicitRank::None; }
                return c->ded.unify(pt, def, true);
            }
            return self._rank_literal(a, pt);
        }

        if a->ty == &null { return ImplicitRank::None; }
        if pt->is_dependent() { return c->ded.unify(pt, a->ty, true); }
        return self.conv->implicit(a->ty, pt);
    }

    priv fn _rank_literal(self, a: *ArgInfo, pt: *Type) -> ImplicitRank {
        var target = self.tu.strip_nullable(pt);
        switch a->lit {
            case LitKind::Int {
                var k = self.tu.kind_of(target);
                if k == BuiltinKind::COUNT { return ImplicitRank::None; }
                if builtin_is_float(k) { return ImplicitRank::Exact; }   // §5: the literal takes the type.
                var pb = self.tu.builtin(BuiltinKind::USize)->size_bits;
                return (ImplicitRank::Exact)
                       if (literal_fits_int(a->lit_lo, a->lit_hi, a->lit_big, a->lit_neg, k, pb))
                       else (ImplicitRank::None);
            }
            case LitKind::Float {
                return (ImplicitRank::Exact) if (self.tu.is_float(target)) else (ImplicitRank::None);
            }
            case LitKind::Null {
                return (ImplicitRank::Exact) if (self.tu.nullable_inner(pt) != &null) else (ImplicitRank::None);
            }
            case LitKind::NullPtr {
                return (ImplicitRank::Exact) if (self.tu.pointee(target) != &null) else (ImplicitRank::None);
            }
            default { return ImplicitRank::None; }
        }
    }

    priv fn _literal_default(self, a: *ArgInfo) -> *Type {
        switch a->lit {
            case LitKind::Int {
                var pb = self.tu.builtin(BuiltinKind::USize)->size_bits;
                if literal_fits_int(a->lit_lo, a->lit_hi, a->lit_big, a->lit_neg, BuiltinKind::I32, pb) { return self.tu.builtin(BuiltinKind::I32); }
                if literal_fits_int(a->lit_lo, a->lit_hi, a->lit_big, a->lit_neg, BuiltinKind::I64, pb) { return self.tu.builtin(BuiltinKind::I64); }
                if literal_fits_int(a->lit_lo, a->lit_hi, a->lit_big, a->lit_neg, BuiltinKind::U64, pb) { return self.tu.builtin(BuiltinKind::U64); }
                if literal_fits_int(a->lit_lo, a->lit_hi, a->lit_big, a->lit_neg, BuiltinKind::I128, pb) { return self.tu.builtin(BuiltinKind::I128); }
                return self.tu.builtin(BuiltinKind::U128);
            }
            case LitKind::Float { return self.tu.builtin(BuiltinKind::F64); }
            default { return &null; }
        }
    }

    // === diagnostics ========================================================

    priv fn _report_no_viable(self, cs: *vec::<Candidate>, at: SourceRange, name: string) {
        var b = self.sc->diag->report(DiagID::SC001E, at.start,
            f"no matching overload for call to '{name}\x27"
        );
        b.first(at);
        var shown: usize = 0;
        for var i: usize = 0; (i < cs->size()) && (shown < 6); ++i {
            var c = &((*cs)[i]);
            b.note(c->f->range.start, f"candidate not viable: {c->reason}").first(c->f->range);
            shown += 1;
        }
        if cs->size() > shown { b.note(at.start, f"and {cs->size() - shown} more candidate(s)"); }
        b.emit();
    }

    priv fn _report_ambiguous(self, cs: *vec::<Candidate>, winners: *vec::<usize>, at: SourceRange, name: string) {
        var b = self.sc->diag->report(DiagID::SC001E, at.start,
            f"call to '{name}' is ambiguous: more than one candidate matches at the same rank"
        );
        b.first(at);
        for var i: usize = 0; (i < winners->size()) && (i < 6); ++i {
            var c = &((*cs)[(*winners)[i]]);
            b.note(c->f->range.start, "candidate declared here").first(c->f->range);
        }
        b.note(at.start, "conversions are not ordered within a rank; cast one argument to make the choice explicit");
        b.emit();
    }
}