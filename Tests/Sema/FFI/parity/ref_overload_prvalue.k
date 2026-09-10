// rule: a PRVALUE argument into `(const T&, T&&)` binds `T&&`
// ([over.ics.rank]/3.2.3). Both overloads accept it, so the verdict alone
// cannot see a wrong pick; each overload returns a different type, and the
// SEMA check pins the callee through `r`. The oracle static_asserts the same.

ffi "c++" import "../ref_overload.h"

fn use_prvalue() {
    var r = f(T())
}

// SEMA:      -- inferred decls --
// SEMA-NEXT: r : f64

// --- lit wiring -----------------------------------------------------------
// PARITY CASE. The .cpp beside this file is the ORACLE: the assertion is that
// kairo and clang reach the same verdict. See Tests/parity_check.py.
//
// XFAIL until value categories reach overload resolution: today a literal or
// temporary against `const T&` ranks Exact and against `T&&` Converted, so
// kairo picks `f(const T&)` -> i32 where clang picks `f(T&&)` -> double.
// Ticket: "value categories in overload resolution" (ExprTyper categories,
// ArgInfo category, Conversion reference binding, && / & tie-breaks).
//
// RUN: %parity %s
// RUN: { %kairo %s --type-check-only --print-sema 2>/dev/null || true; } | %FileCheck %s --check-prefix=SEMA
// REQUIRES: clang
// XFAIL: *
