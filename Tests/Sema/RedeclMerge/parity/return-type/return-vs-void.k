// rule: a declared return type against an omitted (void) one is a
// mismatch. The case that goes silently unreported if the null-canonical
// early-return in _check_return_agrees fires too eagerly.
// EXPECT-ERROR: 5: fn-return-type-differs

fn k(i32) -> i32
fn k(x: i32) { }
