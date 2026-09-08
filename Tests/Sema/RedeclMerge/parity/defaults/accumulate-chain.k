// ported: clang/test/CXX/dcl.decl/dcl.meaning/dcl.fct.default/p2.cpp
// rule: defaults ACCUMULATE across a chain, one specification each. The
// third decl may give i a default only because j and k already have one.
// Pairwise comparison in _merge_cell must reach decl 1 from decl 5, not
// only the immediately previous link.
// EXPECT-ERROR: 12: default-arg-redefinition

fn f0(i: i32, j: i32, k: i32 = 3)
fn f0(i: i32, j: i32, k: i32)
fn f0(i: i32, j: i32 = 2, k: i32)
fn f0(i: i32 = 1, j: i32, k: i32)
fn f0(i: i32, j: i32, k: i32)
fn f0(i: i32 = 1, j: i32, k: i32)
