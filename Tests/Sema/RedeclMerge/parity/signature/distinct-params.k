// rule: different parameter types are a genuine overload. Must NOT link,
// must NOT error. The negative half of _same_signature.
// EXPECT-CLEAN

fn h(x: i32) { }
fn h(x: i64) { }
fn h(x: *i32) { }
fn h(x: i32, y: i32) { }
