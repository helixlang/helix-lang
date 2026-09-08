// ported: clang/test/SemaCXX/overload-decl.cpp, function-redecl-2.cpp
// rule: return type is NOT in the signature, which is exactly why a
// mismatch is a conflict rather than an overload.
// EXPECT-ERROR: 7: fn-return-type-differs

type Float = f32

fn f(i32, Float)
fn f(i32, Float) -> i32
