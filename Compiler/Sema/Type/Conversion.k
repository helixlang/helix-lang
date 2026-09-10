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