// rule: an omitted return type and an explicit -> void are the SAME
// type. If return_type is null for one and a void node for the other,
// _check_return_agrees must normalize before comparing.
// EXPECT-CLEAN

fn a(i32)
fn a(x: i32) -> void { }

fn b(i32) -> void
fn b(x: i32) { }
