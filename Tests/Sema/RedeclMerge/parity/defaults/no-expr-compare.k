// rule: NOT an expression comparison. Two textually identical defaults
// are still an error, and a default that cannot be evaluated at all is
// still an error. Nothing here is ever folded.
// EXPECT-ERROR: 8: default-arg-redefinition
// EXPECT-ERROR: 13: default-arg-redefinition

fn rng() -> i32 { return 4 }

fn a(x: i32 = 1)
fn a(x: i32 = 1) { }

fn b(x: i32 = rng())
fn b(x: i32 = rng()) { }
