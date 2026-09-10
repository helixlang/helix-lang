// ported: clang/test/CXX/dcl.decl/dcl.meaning/dcl.fct.default/p4.cpp
// rule: the specified-once rule spans the in-class declaration and the
// out-of-line definition. f re-specifies (error); g ADDS a default to a
// parameter that lacked one (legal) -- the rule is per-parameter, not
// per-declaration.
// EXPECT-ERROR: 12: default-arg-redefinition

class C {
    fn f(i: i32 = 3)
    fn g(i: i32, j: i32 = 99)
}

fn C::f(i: i32 = 3) { }

fn C::g(i: i32 = 88, j: i32) { }

// --- lit wiring -----------------------------------------------------------
// Kept at the BOTTOM on purpose: the EXPECT-ERROR annotation above names an
// absolute LINE NUMBER in the code, so anything inserted above the code
// silently invalidates it.
//
// PARITY CASE. The .cpp beside this file is the ORACLE: the assertion is that
// kairo and clang reach the same verdict (both accept, or both reject).
// Message text is deliberately NOT compared -- see Tests/parity_check.py.
// On failure clang is right by definition, unless the difference is a
// deliberate Kairo rule -- in which case the case belongs in kairo-only/.
//
// RUN: %parity %s
// REQUIRES: clang
