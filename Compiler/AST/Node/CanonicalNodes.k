/// --- The Kairo Project ------------------- AST/Node/CanonicalNodes.k --- ///
///
///   \brief The two type nodes the PARSER never produces: BuiltinType and
///          RecordType. Both exist only as canonical forms, created solely by
///          CanonicalTypeStore.
///
///   \detail
///     Every other TypeKind is a piece of SYNTAX -- `*T`, `[T]`, `Foo::Bar<U>`
///     -- and the parser builds it from tokens. These two are not syntax:
///
///       BuiltinType  `i32` lexes as the keyword token AtI32, not as an
///                    identifier, so it is in no symbol table and resolves by
///                    token kind rather than by lookup. It is a universal
///                    entity (Clang's IntTy model): one singleton per kind per
///                    build, owned by the store, referenced by every TU.
///
///       RecordType   the canonical form of a nominal type USE. `Foo` and
///                    `ns::Foo` and an imported `Foo` are three different
///                    ChainType nodes naming ONE entity; they all canonicalize
///                    to the same RecordType(canonical decl, canonical args).
///                    Without it a ChainType's canonical could only point at
///                    another ChainType, and type identity would degrade to
///                    "whichever spelling was resolved first".
///
///   \par Lifetime
///     Allocated from CanonicalTypeStore's own arena, NOT from any
///     ASTContext::ast_alloc. That is load-bearing: release_from(Parsing)
///     resets a fid's ast_alloc, and a canonical shared with every importer
///     cannot die with the file that first mentioned it.
///
///   \par Not parseable
///     Neither has a create_error(): a recovery node stands for a failed
///     PARSE, and nothing parses into these. TypeResolution signals failure by
///     leaving `canonical` null and poisoning the syntax node instead.
///
///   Part of the Kairo Project, under the Apache License v2.0 with the
///   Kairo Runtime Library Exception.
///
///   See: https://www.kairolang.org/LICENSE.txt
///   SPDX-License-Identifier: Apache-2.0 WITH KAIRO-RUNTIME-EXCEPTION
///   Copyright (c) 2026 Dhruvan Kartik
///
/// ------------------------------------------------------------------------ ///

import AST::Node::Base::*;
import AST::ASTSupport::*;
import Token::TokenKind::*;

/// \enum BuiltinKind
/// \brief The primitive value types. One singleton Type node per entry, owned
///        by CanonicalTypeStore and shared build-wide.
///
/// \detail
///   Ordering is arbitrary and is NOT an ABI: the store indexes an array by
///   it, nothing else depends on the numbers. Insert anywhere, keep COUNT
///   last.
///
///   These are not names. A builtin never enters a SymbolTable, never rides
///   the import overlay, and can never be shadowed -- `i32` lexes as the
///   keyword token AtI32 and TypeResolution asks builtin_for_token() before
///   it asks any table anything. That is why `var i32 = 3` cannot break
///   `var x: i32`, and why none of these have a WellKnownType row.
///
/// \par WIDTH IS NOT IDENTITY
///   Three widths now carry more than one entry, and this is the invariant
///   most likely to be violated by accident:
///
///     8 bits   F8E4M3, F8E5M2, I8, U8, Byte
///     16 bits  F16, BF16, I16, U16
///     32 bits  F32, I32, U32, Char
///
///   builtin_fixed_bits() answers "how wide", never "which type". A float
///   comparison keyed on width alone silently conflates F16 with BF16 and
///   F8E4M3 with F8E5M2 -- neither pair is interconvertible without
///   rounding, and neither is the other's canonical. Ask
///   builtin_float_format() instead. Same for Byte vs U8 and Char vs U32:
///   same width, distinct types, distinct rules.
///
/// \par Codegen is not uniform, and the enum does not hide it
///   Four different lowering strategies appear below. The type-level view is
///   uniform -- every entry is a BuiltinType singleton with real size_bits
///   and itself as canonical -- but the emitted C++ is not, and the
///   difference is documented per entry rather than inferred from the name:
///
///     native      a C++/LLVM type exists; emit it and stop.
///     legalized   LLVM has the type and splits it; free multi-word codegen.
///     libcall     LLVM has the type and calls compiler-rt; correct, slow.
///     corelib     LLVM has NO type; kairo::rt struct, arithmetic in f32/f64.
///
enum BuiltinKind derives u8 {
    /// The empty type. Not a value type: no storage, no literals, legal only
    /// as a return type and as the pointee of an untyped pointer.
    Void = 0,

    /// One byte in memory, not one bit -- addressability. Valid values are
    /// exactly {0, 1}; bool->integer is ALWAYS zext, never sext. `true` and
    /// `false` are keyword tokens lexed to literal nodes, not shadowable
    /// identifiers, so there is no library enum and no underlying type to
    /// configure. Match exhaustiveness knows it has exactly two inhabitants.
    Bool,

    // === Integers ==========================================================
    //
    // I8..I64 / U8..U64 are native: one register, one instruction.
    //
    // I128..I512 / U128..U512 are LEGALIZED. LLVM supports arbitrary-width
    // integers, so codegen emits `iN` and legalization breaks it into
    // multi-word carry chains using the target's native width -- SIMD where
    // profitable, all for free, on every architecture. No corelib code
    // exists for these and none is needed. The cost is instruction count,
    // not calls: roughly 2-4x at 128 bits, 8-16x at 256, 20-40x at 512
    // versus i64. Document that in the language docs; it is real, and it is
    // still faster than any heap-limb bignum an ecosystem crate would give.

    I8, I16, I32, I64, I128, I256, I512,
    U8, U16, U32, U64, U128, U256, U512,

    /// Pointer-width integers. Their size is TARGET-dependent, so unlike the
    /// fixed-width entries above their size_bits is filled from the target
    /// triple at store construction, not from the name. builtin_fixed_bits()
    /// returns 0 for these two on purpose -- 0 means "ask the store".
    USize, ISize,

    // === 8-bit floats ======================================================
    //
    // OCP 8-bit Floating Point Specification. Two formats, and the split is
    // not arbitrary: E4M3 spends its bits on precision (weights and
    // activations), E5M2 spends them on range and mirrors F16's exponent
    // field exactly, which makes F16<->E5M2 conversion a shift rather than a
    // renormalization (gradients). Neither dominates; both ship.
    //
    // NO IR TYPE EXISTS. Verified against this tree: llvm/IR/Type.h's TypeID
    // enum runs Half, BFloat, Float, Double, X86_FP80, FP128, PPC_FP128 and
    // stops. There is no Float8*TyID. So:
    //
    //   storage      kairo::rt::f8e4m3 / f8e5m2 -- a struct wrapping u8.
    //   arithmetic   promote to f32, compute, narrow back. See
    //                builtin_arith_promote().
    //   folding      APFloat HAS the semantics even though IR does not
    //                (semFloat8E4M3FN, semFloat8E5M2). The constant
    //                evaluator uses them directly, so eval-time arithmetic
    //                needs no bit-twiddling of ours.
    //
    // The runtime header and APFloat MUST agree on all 256 bit patterns in
    // both directions. They are two implementations of one format, and a
    // disagreement is a miscompile that appears only when a value crosses
    // between compile time and run time. Test it exhaustively; 512 cases.
    //
    // NOT INCLUDED, deliberately:
    //   Float8E4M3      the true-IEEE E4M3 with infinities, max finite 240.
    //                   Nothing ships it. F8E4M3 below binds to the FN form.
    //   *FNUZ variants  AMD MI300 (bias 8 / 16) and Graphcore (bias 11).
    //                   Each is one vendor's ABI. A Kairo program spelling
    //                   `f8e4m3` must denote the same format on every target
    //                   it compiles for; these belong behind FFI, not here.
    //   Float8E8M0FNU   8 exponent bits, no sign, no mantissa. A scale factor
    //                   for MXFP4/MXFP8 blocks, not a value type: it has no
    //                   arithmetic and CANNOT REPRESENT ZERO, which would
    //                   break zero-initialization outright. APFloat
    //                   special-cases it out of hasZero() for the same
    //                   reason.

    /// `f8e4m3`. OCP E4M3 in its FINITE-ONLY form: 1 sign / 4 exponent /
    /// 3 mantissa, bias 7, max finite 448, no infinities, NaN is the
    /// all-ones significand alone. Binds to APFloat's semFloat8E4M3FN --
    /// NOT semFloat8E4M3, which is the IEEE variant with infinities and a
    /// max finite of 240. The names differ by two characters and the
    /// results differ from every ML toolchain in existence.
    F8E4M3,

    /// `f8e5m2`. OCP E5M2: 1/5/2, bias 15, IEEE-shaped -- infinities and
    /// NaN both present. Exponent field is identical to F16's.
    /// APFloat: semFloat8E5M2.
    F8E5M2,

    // === 16-bit floats =====================================================
    //
    // Two DIFFERENT formats at one width. Both are native IR types in this
    // tree (HalfTyID, BFloatTyID); neither converts to the other without
    // rounding; neither is the other's canonical.

    /// `f16`. IEEE 754 binary16: 1/5/10, bias 15. A FULL ARITHMETIC TYPE,
    /// not storage-only -- `T + T` must mean addition for every T a generic
    /// accepts, and a storage-only float poisons that. Emit LLVM `half` and
    /// let legalization promote to f32 on targets without native ALU
    /// support (pre-Sapphire-Rapids x86, pre-ARMv8.2). Same policy already
    /// accepted for F128's libcalls: correctness at the type level, cost
    /// disclosed in the docs. C++: _Float16.
    F16,

    /// `bf16`. bfloat16: 1/8/7, bias 127 -- f32's exponent range with the
    /// mantissa truncated, so f32->bf16 is a truncation and bf16->f32 is
    /// exact. Native hardware on AVX512-BF16 and ARM BF16; native IR type
    /// (BFloatTyID). The one new float here that costs nothing.
    /// C++: __bf16. APFloat: semBFloat.
    ///
    /// Breaks the f<width> naming scheme on purpose. `bf16` is what every
    /// ML toolchain, every paper and every hardware manual calls it;
    /// a scheme-pure invention would be unrecognizable for no gain.
    BF16,

    // === 32/64-bit floats ==================================================

    /// `f32`. IEEE binary32. C++: float. APFloat: semIEEEsingle.
    F32,

    /// `f64`. IEEE binary64. The DEFAULT for float literals.
    /// C++: double. APFloat: semIEEEdouble.
    F64,

    /// `f80`. x87 80-bit extended: 1/15/64 with an EXPLICIT integer bit,
    /// which no IEEE interchange format has -- it is not "binary80" and
    /// calling it that will eventually mislead someone writing codegen.
    ///
    /// X86 ONLY. There is no x86_fp80 on ARM, RISC-V or anything else, and
    /// on those targets C's `long double` is just double. TypeResolution
    /// emits a HARD ERROR naming the target triple when this is spelled for
    /// a non-x86 target, rather than silently widening to F128 or narrowing
    /// to F64 -- a quiet precision change is worse than a failed build.
    ///
    /// Retained despite being the one non-portable entry because the
    /// compiler's own Immediate::FloatData stores literal values in it.
    /// C++: long double (x86 only). APFloat: semX87DoubleExtended.
    F80,

    /// `f128`. IEEE binary128 (quadruple): 1/15/112. LLVM has the type
    /// (FP128TyID) but NO mainstream CPU has the instructions, so every
    /// operation lowers to a compiler-rt LIBCALL -- __addtf3, __multf3,
    /// __divtf3. Roughly 10-50x f64 depending on the operation. Correct
    /// everywhere, fast nowhere.
    ///
    /// Do not confuse with PPC_FP128TyID, LLVM's OTHER 128-bit float: a
    /// double-double pair, structurally what F256 is below at half the limb
    /// count. Kairo's f128 is always FP128TyID; a PowerPC `long double`
    /// arriving through FFI is a different type and the FFI mapping table
    /// must say so.
    /// C++: __float128. APFloat: semIEEEquad.
    F128,

    // === Extended-width floats =============================================
    //
    // LLVM HAS NO TYPE AT THESE WIDTHS. Unlike the integers above, there is
    // nothing to legalize and nothing to call: these are the only entries in
    // this enum that need Kairo implementation code. corelib soft-float,
    // four and eight f64 limbs, Dekker arithmetic for add/mul and
    // Newton-Raphson for div and sqrt. Not IEEE anything -- the limb
    // representation has no single exponent field -- so builtin_float_format
    // reports SoftDouble4 / SoftDouble8 rather than a binaryN.

    /// `f256`. Four f64 limbs. C++: kairo::rt::f256.
    F256,

    /// `f512`. Eight f64 limbs. C++: kairo::rt::f512.
    F512,

    // === Non-numeric scalars ===============================================

    /// `byte`. A raw octet. Same width as U8 and NOT THE SAME TYPE: bitwise
    /// operators only, arithmetic is a diagnostic. This is the distinction
    /// C++ never made -- char, unsigned char and uint8_t are one type there,
    /// so "storage" and "small number" are indistinguishable to the type
    /// system. Here they are not. C++: std::byte.
    Byte,

    /// `char`. A decoded Unicode scalar value, 4 bytes. Same width as U32
    /// and NOT THE SAME TYPE. Strings are UTF-8 backed by u8; `char` appears
    /// when a single codepoint is wanted -- s[i] yields one, `for ch in s`
    /// yields them. A [char; N] is 4x the UTF-8 size, bought for O(1)
    /// codepoint indexing. C++: char32_t.
    Char,

    COUNT,   ///< keep last.
}

/// \brief Map a type-position token kind to its builtin, or COUNT when the
///        token does not name one.
///
/// \details This is the WHOLE of builtin "lookup". `i32` is not a name: it
///          never enters a SymbolTable, never rides the import overlay, and is
///          never shadowable. TypeResolution asks this question of a
///          ChainType's first segment before it asks the symbol tables
///          anything, which is also why `var i32 = 3` cannot break `var x: i32`.
fn builtin_for_token(k: TokenKind) -> BuiltinKind {
    switch k {
        case TokenKind::AtVoid  { return BuiltinKind::Void;  }
        case TokenKind::AtBool  { return BuiltinKind::Bool;  }
        case TokenKind::AtByte  { return BuiltinKind::Byte;  }
        case TokenKind::AtChar  { return BuiltinKind::Char;  }

        case TokenKind::AtI8    { return BuiltinKind::I8;    }
        case TokenKind::AtI16   { return BuiltinKind::I16;   }
        case TokenKind::AtI32   { return BuiltinKind::I32;   }
        case TokenKind::AtI64   { return BuiltinKind::I64;   }
        case TokenKind::AtI128  { return BuiltinKind::I128;  }
        case TokenKind::AtI256  { return BuiltinKind::I256;  }
        case TokenKind::AtI512  { return BuiltinKind::I512;  }

        case TokenKind::AtU8    { return BuiltinKind::U8;    }
        case TokenKind::AtU16   { return BuiltinKind::U16;   }
        case TokenKind::AtU32   { return BuiltinKind::U32;   }
        case TokenKind::AtU64   { return BuiltinKind::U64;   }
        case TokenKind::AtU128  { return BuiltinKind::U128;  }
        case TokenKind::AtU256  { return BuiltinKind::U256;  }
        case TokenKind::AtU512  { return BuiltinKind::U512;  }

        case TokenKind::AtUsize { return BuiltinKind::USize; }
        case TokenKind::AtIsize { return BuiltinKind::ISize; }

        case TokenKind::AtBf16   { return BuiltinKind::BF16;  }
        case TokenKind::AtF16    { return BuiltinKind::F16;   }
        case TokenKind::AtF32    { return BuiltinKind::F32;   }
        case TokenKind::AtF64    { return BuiltinKind::F64;   }
        case TokenKind::AtF80    { return BuiltinKind::F80;   }
        case TokenKind::AtF128   { return BuiltinKind::F128;  }
        case TokenKind::AtF256   { return BuiltinKind::F256;  }
        case TokenKind::AtF512   { return BuiltinKind::F512;  }
        case TokenKind::AtF8E4M3 { return BuiltinKind::F8E4M3; }
        case TokenKind::AtF8E5M2 { return BuiltinKind::F8E5M2; }

        default { return BuiltinKind::COUNT; }
    }
}

/// \brief The C++ spelling a builtin lowers to. THE single source of truth
///        for emission -- codegen, the FFI mapping table and the header
///        emitter all read this, so a type has one spelling or none.
///
/// \detail
///   Four categories, and the return value does not distinguish them because
///   the emitter does not care: it prints the string. What it means for a
///   caller is documented per row above in BuiltinKind.
///
///     keyword / native   void, bool, float, double, _Float16, __bf16
///     &lt;cstdint&gt; alias    the fixed-width integers; ::std:: qualified so a
///                        user `using namespace std` cannot capture them.
///     _BitInt(N)         i128..i512 / u128..u512. NOT an alias of anything
///                        -- LLVM legalizes iN directly and this is the C++
///                        surface for it.
///     kairo::rt struct   the two fp8 entries and f256/f512, where no C++ or
///                        LLVM type exists at all.
///
///   USize/ISize are the only target-dependent rows. `usize`/`isize` map to
///   size_t/ptrdiff_t rather than a fixed width, so the width follows the
///   triple the same way size_bits does.
///
///   F80 is x86-only. TypeResolution rejects it for other targets before
///   codegen ever asks, so this returns the spelling unconditionally rather
///   than duplicating the target check.
///
/// \return the spelling, or "" for COUNT and any unhandled value. An empty
///         string is a codegen bug, never a valid emission.
fn builtin_cxx_spelling(b: BuiltinKind) -> string {
    switch b {
        case BuiltinKind::Void  { return "void"; }
        case BuiltinKind::Bool  { return "bool"; }

        // --- integers: fixed width ---------------------------------------
        case BuiltinKind::I8    { return "::std::int8_t";   }
        case BuiltinKind::I16   { return "::std::int16_t";  }
        case BuiltinKind::I32   { return "::std::int32_t";  }
        case BuiltinKind::I64   { return "::std::int64_t";  }
        case BuiltinKind::U8    { return "::std::uint8_t";  }
        case BuiltinKind::U16   { return "::std::uint16_t"; }
        case BuiltinKind::U32   { return "::std::uint32_t"; }
        case BuiltinKind::U64   { return "::std::uint64_t"; }

        // --- integers: extended width ------------------------------------
        // _BitInt is C23, adopted by clang for C++ as an extension. Clang
        // lowers these to LLVM iN and legalizes to carry chains.
        case BuiltinKind::I128  { return "_BitInt(128)";           }
        case BuiltinKind::I256  { return "_BitInt(256)";           }
        case BuiltinKind::I512  { return "_BitInt(512)";           }
        case BuiltinKind::U128  { return "unsigned _BitInt(128)";  }
        case BuiltinKind::U256  { return "unsigned _BitInt(256)";  }
        case BuiltinKind::U512  { return "unsigned _BitInt(512)";  }

        // --- integers: target width ---------------------------------------
        case BuiltinKind::USize { return "::std::size_t";    }
        case BuiltinKind::ISize { return "::std::ptrdiff_t"; }

        // --- floats: no C++ type exists -----------------------------------
        // u8-wrapping structs. Arithmetic promotes to float in the header,
        // matching what fpext/fptrunc would emit if IR had the type.
        case BuiltinKind::F8E4M3 { return "::kairo::rt::f8e4m3"; }
        case BuiltinKind::F8E5M2 { return "::kairo::rt::f8e5m2"; }

        // --- floats: native ------------------------------------------------
        case BuiltinKind::F16   { return "_Float16"; }
        case BuiltinKind::BF16  { return "__bf16";   }
        case BuiltinKind::F32   { return "float";    }
        case BuiltinKind::F64   { return "double";   }

        // x86 only; see BuiltinKind::F80. On a non-x86 target `long double`
        // is double, which is exactly why T rejects f80 there instead of
        // letting this row silently mean something else.
        case BuiltinKind::F80   { return "long double"; }

        case BuiltinKind::F128  { return "__float128"; }

        // --- floats: corelib soft-float -----------------------------------
        case BuiltinKind::F256  { return "::kairo::rt::f256"; }
        case BuiltinKind::F512  { return "::kairo::rt::f512"; }

        // --- non-numeric scalars -------------------------------------------
        // std::byte is a scoped enum in C++: bitwise ops only, arithmetic
        // ill-formed. The C++ side enforces Byte's rule for free.
        case BuiltinKind::Byte  { return "::std::byte"; }
        case BuiltinKind::Char  { return "char32_t";    }

        default { return ""; }
    }
}

/// \brief Source spelling of a builtin, for diagnostics and dumps.
fn builtin_spelling(b: BuiltinKind) -> string {
    switch b {
        case BuiltinKind::Void   { return "void";  }
        case BuiltinKind::Bool   { return "bool";  }
        case BuiltinKind::Byte   { return "byte";  }
        case BuiltinKind::Char   { return "char";  }
        case BuiltinKind::I8     { return "i8";    }
        case BuiltinKind::I16    { return "i16";   }
        case BuiltinKind::I32    { return "i32";   }
        case BuiltinKind::I64    { return "i64";   }
        case BuiltinKind::I128   { return "i128";  }
        case BuiltinKind::I256   { return "i256";  }
        case BuiltinKind::I512   { return "i512";  }
        case BuiltinKind::U8     { return "u8";    }
        case BuiltinKind::U16    { return "u16";   }
        case BuiltinKind::U32    { return "u32";   }
        case BuiltinKind::U64    { return "u64";   }
        case BuiltinKind::U128   { return "u128";  }
        case BuiltinKind::U256   { return "u256";  }
        case BuiltinKind::U512   { return "u512";  }
        case BuiltinKind::USize  { return "usize"; }
        case BuiltinKind::ISize  { return "isize"; }
        case BuiltinKind::F8E4M3 { return "f8e4m3"; }
        case BuiltinKind::F8E5M2 { return "f8e5m2"; }
        case BuiltinKind::BF16   { return "bf16"; }
        case BuiltinKind::F16    { return "f16";   }
        case BuiltinKind::F32    { return "f32";   }
        case BuiltinKind::F64    { return "f64";   }
        case BuiltinKind::F80    { return "f80";   }
        case BuiltinKind::F128   { return "f128";  }
        case BuiltinKind::F256   { return "f256";  }
        case BuiltinKind::F512   { return "f512";  }
        default                  { return "<builtin?>"; }
    }
}

/// \brief Width in bits, or 0 for the two target-dependent entries (the store
///        fills those from the target triple).
fn builtin_fixed_bits(b: BuiltinKind) -> usize {
    switch b {
        case BuiltinKind::Void   { return 0;   }
        case BuiltinKind::Bool   { return 8;   }
        case BuiltinKind::Byte   { return 8;   }
        case BuiltinKind::Char   { return 32;  }
        case BuiltinKind::I8     { return 8;   }
        case BuiltinKind::I16    { return 16;  }
        case BuiltinKind::I32    { return 32;  }
        case BuiltinKind::I64    { return 64;  }
        case BuiltinKind::I128   { return 128; }
        case BuiltinKind::I256   { return 256; }
        case BuiltinKind::I512   { return 512; }
        case BuiltinKind::U8     { return 8;   }
        case BuiltinKind::U16    { return 16;  }
        case BuiltinKind::U32    { return 32;  }
        case BuiltinKind::U64    { return 64;  }
        case BuiltinKind::U128   { return 128; }
        case BuiltinKind::U256   { return 256; }
        case BuiltinKind::U512   { return 512; }
        case BuiltinKind::USize  { return 0;   }   // target-dependent.
        case BuiltinKind::ISize  { return 0;   }   // target-dependent.
        case BuiltinKind::F8E4M3 { return 8;   }
        case BuiltinKind::F8E5M2 { return 8;   }
        case BuiltinKind::BF16   { return 16;  }
        case BuiltinKind::F16    { return 16;  }
        case BuiltinKind::F32    { return 32;  }
        case BuiltinKind::F64    { return 64;  }
        case BuiltinKind::F80    { return 80;  }
        case BuiltinKind::F128   { return 128; }
        case BuiltinKind::F256   { return 256; }
        case BuiltinKind::F512   { return 512; }
        default                  { return 0;   }   // USize / ISize: target-dependent.
    }
}

fn builtin_is_integer(b: BuiltinKind) -> bool {
    var v = static_cast::<u8>(b);
    return ((v >= static_cast::<u8>(BuiltinKind::I8))
         && (v <= static_cast::<u8>(BuiltinKind::U512)))
        || (b == BuiltinKind::USize) || (b == BuiltinKind::ISize);
}

fn builtin_is_signed(b: BuiltinKind) -> bool {
    var v = static_cast::<u8>(b);
    return ((v >= static_cast::<u8>(BuiltinKind::I8))
         && (v <= static_cast::<u8>(BuiltinKind::I512)))
        || (b == BuiltinKind::ISize);
}

fn builtin_is_float(b: BuiltinKind) -> bool {
    var v = static_cast::<u8>(b);
    return (v >= static_cast::<u8>(BuiltinKind::F8E4M3))
        && (v <= static_cast::<u8>(BuiltinKind::F512));
}

///
///  BuiltinType
///  i32 | bool | f64 | void | usize ...
///
///  A singleton. `canonical` points at ITSELF: a builtin is already fully
///  desugared, and self-canonicality is what lets the identity test
///  (a->canonical == b->canonical) work uniformly without a builtin special
///  case at every comparison site.
///
class BuiltinType derives Type {
    pub var builtin: BuiltinKind = BuiltinKind::Void;

    /* OVERRIDE */ static fn classof(ptr: *Type) -> bool {
        return ptr->kind == TypeKind::BuiltinType;
    }

    /* OVERRIDE */ static fn type_kind() -> TypeKind {
        return TypeKind::BuiltinType;
    }

    fn spelling(self) -> string { return builtin_spelling(self.builtin); }
}

///
///  RecordType
///  The canonical form of a nominal type use: Foo | Foo<i32> | ns::Inner
///
///  `decl` is the CANONICAL link of the entity's redecl chain (the definition
///  once N(a) retargeted it), never whichever forward declaration a particular
///  spelling happened to bind. That is what collapses `class Foo;` and
///  `class Foo { }` to one type.
///
///  `args` holds CANONICAL generic arguments, empty for a non-generic type.
///  Uniquing is on (decl, args...), so Vec<i32> and Vec<u8> are distinct
///  entities while two spellings of Vec<i32> are one.
///
class RecordType derives Type {
    pub var decl: *Decl = &null;          ///< canonical Class/Struct/Union/Interface/EnumDecl.
    pub var args: FrozenList::<*Type>;    ///< canonical generic args; empty if non-generic.

    /* OVERRIDE */ static fn classof(ptr: *Type) -> bool {
        return ptr->kind == TypeKind::RecordType;
    }

    /* OVERRIDE */ static fn type_kind() -> TypeKind {
        return TypeKind::RecordType;
    }

    fn is_generic_instance(self) -> bool { return self.args.size() > 0; }
}