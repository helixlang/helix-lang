// rule: the default lives on the declaration; a later definition that
// omits it is correct, not a conflict. The negative half.
// EXPECT-CLEAN

fn f(x: i32 = 1)
fn f(x: i32) { }
