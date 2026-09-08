// rule: signature identity is CANONICAL, not syntactic. An alias names
// the same entity, so these link rather than overload.
// EXPECT-ERROR: 8: fn-redefinition

type Int = i32

fn g(x: i32) { }
fn g(x: Int) { }
