// ported: clang/test/SemaCXX/default1.cpp
// rule: a default may be SPECIFIED once per parameter per scope. The
// first declaration having none is fine; the error is on the second
// specification, not on disagreement.
// EXPECT-ERROR: 7: default-arg-redefinition

fn f(i32)
fn f(x: i32 = 0)
fn f(x: i32 = 17)
