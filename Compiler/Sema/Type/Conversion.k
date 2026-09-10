/// --- The Kairo Project ---------------------- Sema/Type/Conversion.k --- ///
///
///   \brief THE conversion relation. One concept in the whole language:
///          classify(S, T) over primitives per the Primitive Conversion
///          Lattice, plus the three non-primitive implicit forms
///          (`T -> T?`, `*Derived -> *Base`, `! -> anything`).
///
///   \detail
///     Two ranks only, and no ordering inside rank 2 (lattice §7): Exact is
///     I, Converted is everything W-shaped. `-fno-implicit-conv` demotes W to
///     E here and nowhere else, which is what makes the flag a strict subset:
///     it can remove a candidate, never reorder two.
///
///     The primitive relation is rule-based here (R1-R5, §4). The lattice doc
///     asks for a table-generated `classify` with property tests; that is a
///     tooling follow-up and the rules below are the table's oracle until
///     then. Every ordered pair returns exactly one class; there is no
///     `default` fallthrough that could hide a missing cell.
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
import AST::Node::Base::*;
import AST::Node::Types::*;
import AST::Node::CanonicalNodes::*;
import AST::Node::Casting::*;
import AST::Kind::TypeKind::*;

/// \enum ConvClass
/// \brief Lattice §1. The class of an ordered primitive pair.
enum ConvClass derives u8 {
    I = 0,  ///< identity.
    W,      ///< implicit: value-preserving, target-independent.
    E,      ///< explicit: `as` required.
    U,      ///< `as` inside an unsafe context.
    X,      ///< no cast exists.
}

/// \enum ImplicitRank
/// \brief What a call site / assignment gets from an implicit sequence.
enum ImplicitRank derives u8 {
    Exact = 0,
    Converted,
    None,
}

/// \enum RefRel
/// \brief How an argument's VALUE type relates to a reference parameter's
///        referee, before its value category is consulted (ref_binding).
enum RefRel derives u8 {
    None = 0,   ///< binds at no category.
    Same,       ///< the referee itself.
    Derived,    ///< a class derived from the referee: binds the base subobject.
    Converts,   ///< converts implicitly: binding goes through a temporary.
}

/// Can an untyped integer literal of the given magnitude take builtin \p bk?
/// \p negative is the unary-minus fold; the magnitude then may reach 2^(bits-1).
fn literal_fits_int(lo: u64, hi: u64, big: bool, negative: bool, bk: BuiltinKind, ptr_bits: usize) -> bool {
    if !builtin_is_integer(bk) { return false; }
    if big { return false; }
    var bits = builtin_fixed_bits(bk);
    if bits == 0 { bits = ptr_bits; }
    var is_signed = builtin_is_signed(bk);
    if negative && (!is_signed) { return false; }

    // magnitude limit as a 128-bit number. Every shift below is done on a
    // u64 ONE: a bare `1` is an i32, and `1 << 63` on one is undefined
    // behaviour, not a large number.
    var one: u64 = 1;
    if bits >= 128 {
        // anything that fit in 128 bits is fine, except the signed top bit.
        if !is_signed { return true; }
        if bits > 128 { return true; }
        if negative { return (hi < (one << 63)) || ((hi == (one << 63)) && (lo == 0)); }
        return hi < (one << 63);
    }
    if hi != 0 { return false; }
    var limit_bits = (bits - 1) if (is_signed) else (bits);
    if limit_bits >= 64 { return true; }
    var limit: u64 = (one << limit_bits);   // exclusive for non-negative
    if negative { return lo <= limit; }
    return lo < limit;
}

class Conversion {
    pub var sc: *SemaContext;
    pub var tu: TypeUtil;

    fn Conversion(self) { self.sc = &null; }
    fn Conversion(self, sc: *SemaContext) {
        self.sc = sc;
        self.tu = TypeUtil(sc);
    }

    fn implicit_enabled(self) -> bool {
        return self.sc->opts.on(self.sc->opts.implicit_conv, true);
    }

    // === primitive lattice ==================================================

    fn classify(self, s: BuiltinKind, t: BuiltinKind) -> ConvClass {
        if s == t { return ConvClass::I; }
        if (s == BuiltinKind::Void) || (t == BuiltinKind::Void) { return ConvClass::X; }
        if (s == BuiltinKind::Never) { return ConvClass::W; }
        if (t == BuiltinKind::Never) { return ConvClass::X; }

        var si = builtin_is_integer(s);
        var ti = builtin_is_integer(t);
        var sf = builtin_is_float(s);
        var tf = builtin_is_float(t);

        if si && ti { return self._int_int(s, t); }
        if si && tf { return self._int_float(s, t); }
        if sf && ti { return ConvClass::E; }
        if sf && tf { return self._float_float(s, t); }

        // non-numeric primitives: §4.
        if s == BuiltinKind::Bool { return (ConvClass::E) if (ti) else (ConvClass::X); }
        if t == BuiltinKind::Bool { return (ConvClass::E) if (si) else (ConvClass::X); }
        if s == BuiltinKind::Char { return (ConvClass::E) if (ti) else (ConvClass::X); }
        if t == BuiltinKind::Char { return (ConvClass::E) if (si) else (ConvClass::X); }
        if s == BuiltinKind::Byte { return (ConvClass::E) if (ti) else (ConvClass::X); }
        if t == BuiltinKind::Byte { return (ConvClass::E) if (si) else (ConvClass::X); }
        return ConvClass::X;
    }

    priv fn _is_ptr_int(self, b: BuiltinKind) -> bool {
        return (b == BuiltinKind::USize) || (b == BuiltinKind::ISize);
    }

    priv fn _int_int(self, s: BuiltinKind, t: BuiltinKind) -> ConvClass {
        if self._is_ptr_int(s) || self._is_ptr_int(t) { return ConvClass::E; }   // §4
        var sb = builtin_fixed_bits(s);
        var tb = builtin_fixed_bits(t);
        if (sb > 64) || (tb > 64) { return ConvClass::E; }                       // R5
        var ss = builtin_is_signed(s);
        var ts = builtin_is_signed(t);
        if (!ss) && (!ts) { return (ConvClass::W) if (tb > sb) else (ConvClass::E); } // R1
        if ss && ts       { return (ConvClass::W) if (tb > sb) else (ConvClass::E); } // R1
        if (!ss) && ts    { return (ConvClass::W) if (tb > sb) else (ConvClass::E); } // R2 uN->iM
        return ConvClass::E;                                                     // R2 iN->uM never
    }

    priv fn _mantissa(self, f: BuiltinKind) -> usize {
        switch f {
            case BuiltinKind::F16  { return 11; }
            case BuiltinKind::BF16 { return 8;  }
            case BuiltinKind::F32  { return 24; }
            case BuiltinKind::F64  { return 53; }
            default                { return 0;  }   // f8*, f80, f128+: never a W target (R5).
        }
    }

    priv fn _int_float(self, s: BuiltinKind, t: BuiltinKind) -> ConvClass {
        if self._is_ptr_int(s) { return ConvClass::E; }
        var sb = builtin_fixed_bits(s);
        if sb > 64 { return ConvClass::E; }
        var m = self._mantissa(t);
        if m == 0 { return ConvClass::E; }
        return (ConvClass::W) if (sb <= m) else (ConvClass::E);                 // R3
    }

    priv fn _float_float(self, s: BuiltinKind, t: BuiltinKind) -> ConvClass {
        // R4, plus bf16 -> f32/f64 which is exact by construction.
        if (s == BuiltinKind::F16) && ((t == BuiltinKind::F32) || (t == BuiltinKind::F64)) { return ConvClass::W; }
        if (s == BuiltinKind::BF16) && ((t == BuiltinKind::F32) || (t == BuiltinKind::F64)) { return ConvClass::W; }
        if (s == BuiltinKind::F32) && (t == BuiltinKind::F64) { return ConvClass::W; }
        return ConvClass::E;
    }

    // === the implicit sequence ==============================================

    /// Rank of \p from -> \p to as an implicit conversion. Both canonical.
    fn implicit(self, from: *Type, to: *Type) -> ImplicitRank {
        if (from == &null) || (to == &null) { return ImplicitRank::None; }
        if from == to { return ImplicitRank::Exact; }
        if self.tu.is_never(from) { return ImplicitRank::Converted; }

        // C++ references. Only imported signatures carry them (Kairo has no
        // spelling), so this is the rule every `const T&` parameter in a
        // header is judged by.
        if isa::<CXXLValueRefType>(to) || isa::<CXXRValueRefType>(to) { return self._bind_ref(from, to); }
        // A reference READ as a value is its referee (lvalue-to-rvalue): a
        // `const T&` return converts wherever a T would.
        if isa::<CXXLValueRefType>(from) { return self.implicit(cast::<CXXLValueRefType>(from)->referee, to); }
        if isa::<CXXRValueRefType>(from) { return self.implicit(cast::<CXXRValueRefType>(from)->referee, to); }

        // primitives
        var fk = self.tu.kind_of(from);
        var tk = self.tu.kind_of(to);
        if (fk != BuiltinKind::COUNT) && (tk != BuiltinKind::COUNT) {
            if self.classify(fk, tk) == ConvClass::W {
                return (ImplicitRank::Converted) if (self.implicit_enabled()) else (ImplicitRank::None);
            }
            return ImplicitRank::None;
        }

        // T -> T?  (and *D -> *B?, i32 -> i64?, both via the inner)
        var to_inner = self.tu.nullable_inner(to);
        if to_inner != &null {
            var from_inner = self.tu.nullable_inner(from);
            if from_inner != &null {
                // T? -> U?: only when the inners relate; never Exact (different type).
                return (ImplicitRank::Converted) if (self.implicit(from_inner, to_inner) != ImplicitRank::None)
                       else (ImplicitRank::None);
            }
            return (ImplicitRank::Converted) if (self.implicit(from, to_inner) != ImplicitRank::None)
                   else (ImplicitRank::None);
        }

        // *Derived -> *Base, same safety.
        var fp = self.tu.pointee(from);
        var tp = self.tu.pointee(to);
        if (fp != &null) && (tp != &null) {
            if self.tu.is_unsafe_ptr(from) != self.tu.is_unsafe_ptr(to) { return ImplicitRank::None; }
            var fd = self.tu.record_decl(fp);
            var td = self.tu.record_decl(tp);
            if (fd != &null) && (td != &null) && (fd != td) && self.tu.derives_from(fd, td) {
                return ImplicitRank::Converted;
            }
            return ImplicitRank::None;
        }

        return ImplicitRank::None;
    }

    /// Reference binding, C++'s ranking where Kairo can see it:
    ///
    ///   `T&` / `const T&` <- T         Exact (binding is an identity, not a conversion)
    ///   `T&`              <- const T&  None  (drops const)
    ///   `const T&`        <- U, U->T   Converted (binds a temporary)
    ///   `B&` / `const B&` <- D, D:B    Converted (derived-to-base)
    ///   `T&&`             <- T / U->T  Converted
    ///
    /// Rvalue references rank Converted, never Exact: Conversion sees types,
    /// not value categories, so the C++ tiebreak that sends an lvalue to
    /// `const T&` and a temporary to `T&&` cannot be expressed here. Ranking
    /// `&&` lower makes a `(const T&, T&&)` overload pair resolve to the
    /// const& one for every argument -- a copy where C++ would move, but
    /// never an ambiguity.
    priv fn _bind_ref(self, from: *Type, to: *Type) -> ImplicitRank {
        var src = from;
        var from_const = false;
        if isa::<CXXLValueRefType>(from) {
            from_const = cast::<CXXLValueRefType>(from)->is_const;
            src = cast::<CXXLValueRefType>(from)->referee;
        } else if isa::<CXXRValueRefType>(from) {
            src = cast::<CXXRValueRefType>(from)->referee;
        }

        if isa::<CXXRValueRefType>(to) {
            var rr = cast::<CXXRValueRefType>(to)->referee;
            if (src == rr) || (self.implicit(src, rr) != ImplicitRank::None) { return ImplicitRank::Converted; }
            return ImplicitRank::None;
        }

        var lr       = cast::<CXXLValueRefType>(to);
        var referee  = lr->referee;
        if src == referee {
            if from_const && (!lr->is_const) { return ImplicitRank::None; }
            return ImplicitRank::Exact;
        }
        var fd = self.tu.record_decl(src);
        var td = self.tu.record_decl(referee);
        if (fd != &null) && (td != &null) && (fd != td) && self.tu.derives_from(fd, td) {
            if from_const && (!lr->is_const) { return ImplicitRank::None; }
            return ImplicitRank::Converted;
        }
        if lr->is_const && (self.implicit(src, referee) != ImplicitRank::None) { return ImplicitRank::Converted; }
        return ImplicitRank::None;
    }

    // === reference binding at an argument ([over.ics.ref]) ===================

    /// Binding an ARGUMENT of known value category to \p to, a C++ reference
    /// parameter. [over.ics.ref] reduced to what Kairo has:
    ///
    ///   `T&`        a non-const LVALUE of T                   Exact
    ///   `const T&`  any lvalue or rvalue of T                 Exact
    ///   `T&&`       an RVALUE (prvalue or xvalue) of T        Exact
    ///
    /// and outside that table, today's ranking on the referee: a derived
    /// class binds the base subobject at Converted under the same category
    /// rule, and a value that only CONVERTS to T binds a temporary at
    /// Converted -- viable for `const T&` and `T&&` whatever its category,
    /// never for a plain `T&`.
    ///
    /// Only an argument site knows the category. implicit() -- returns,
    /// initializers, the post-selection re-check -- keeps the category-blind
    /// _bind_ref, which accepts everything this accepts.
    fn ref_binding(self, to: *Type, cat: Expr::ValueCategory, from_const: bool, rel: RefRel) -> ImplicitRank {
        if rel == RefRel::None { return ImplicitRank::None; }
        var rvalue = (cat == Expr::ValueCategory::RValue) || (cat == Expr::ValueCategory::XValue);
        var lvalue = cat == Expr::ValueCategory::LValue;
        var exact  = rel == RefRel::Same;

        if isa::<CXXRValueRefType>(to) {
            if rel == RefRel::Converts { return ImplicitRank::Converted; }   // a temporary is an rvalue.
            if !rvalue { return ImplicitRank::None; }
            return (ImplicitRank::Exact) if (exact) else (ImplicitRank::Converted);
        }
        if !isa::<CXXLValueRefType>(to) { return ImplicitRank::None; }
        if cast::<CXXLValueRefType>(to)->is_const {
            return (ImplicitRank::Exact) if (exact) else (ImplicitRank::Converted);
        }
        // Plain `T&`: a modifiable lvalue, and never a temporary.
        if (!lvalue) || from_const || (rel == RefRel::Converts) { return ImplicitRank::None; }
        return (ImplicitRank::Exact) if (exact) else (ImplicitRank::Converted);
    }

    /// \p src (a value type) against \p referee.
    fn ref_rel(self, src: *Type, referee: *Type) -> RefRel {
        if (src == &null) || (referee == &null) { return RefRel::None; }
        if src == referee { return RefRel::Same; }
        var fd = self.tu.record_decl(src);
        var td = self.tu.record_decl(referee);
        if (fd != &null) && (td != &null) && (fd != td) && self.tu.derives_from(fd, td) { return RefRel::Derived; }
        if self.implicit(src, referee) != ImplicitRank::None { return RefRel::Converts; }
        return RefRel::None;
    }

    /// implicit() for an ARGUMENT whose category is known: a reference
    /// parameter goes through ref_binding, anything else is unchanged.
    fn implicit_arg(self, from: *Type, to: *Type, cat: Expr::ValueCategory, from_const: bool) -> ImplicitRank {
        if (from == &null) || (to == &null) { return ImplicitRank::None; }
        var to_l = isa::<CXXLValueRefType>(to);
        var to_r = isa::<CXXRValueRefType>(to);
        if (!to_l) && (!to_r) { return self.implicit(from, to); }
        // A reference-typed argument's value is its referee, and a `const T&`
        // one is const whatever its arm said.
        var src = from;
        var c   = from_const;
        if isa::<CXXLValueRefType>(from) {
            c   = c || cast::<CXXLValueRefType>(from)->is_const;
            src = cast::<CXXLValueRefType>(from)->referee;
        } else if isa::<CXXRValueRefType>(from) {
            src = cast::<CXXRValueRefType>(from)->referee;
        }
        var referee = (cast::<CXXLValueRefType>(to)->referee) if (to_l) else (cast::<CXXRValueRefType>(to)->referee);
        return self.ref_binding(to, cat, c, self.ref_rel(src, referee));
    }

    fn convertible(self, from: *Type, to: *Type) -> bool {
        return self.implicit(from, to) != ImplicitRank::None;
    }

    /// Branch join for if/match/try: the type both sides convert to, or null.
    fn join(self, a: *Type, b: *Type) -> *Type {
        if a == &null { return b; }
        if b == &null { return a; }
        if a == b { return a; }
        if self.tu.is_never(a) { return b; }
        if self.tu.is_never(b) { return a; }
        var ab = self.implicit(a, b);
        var ba = self.implicit(b, a);
        if (ab != ImplicitRank::None) && (ba == ImplicitRank::None) { return b; }
        if (ba != ImplicitRank::None) && (ab == ImplicitRank::None) { return a; }
        return &null;   // neither or both: the caller errors (both = a W-cycle, invariant 2).
    }
}