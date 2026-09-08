// rule: each out-of-line definition picks its own overload by
// signature. Both link; neither is a redefinition of the other.
// EXPECT-CLEAN

class C {
    fn bump()
    fn bump(n: i32)
}

fn C::bump() { }
fn C::bump(n: i32) { }
