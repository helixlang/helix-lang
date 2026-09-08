// ported: clang/test/SemaCXX/function-overloaded-redecl.cpp
// rule: top-level const on a parameter is NOT part of the signature, and
// an alias is transparent. Four declarations, ONE entity.
// EXPECT-ERROR: 9: fn-redefinition

type cInt = const i32

fn f(i32)
fn f(const i32)

fn f(x: i32) { }
fn f(x: cInt) { }
