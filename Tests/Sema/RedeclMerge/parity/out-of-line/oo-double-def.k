// rule: two out-of-line definitions of one member is a redefinition.
// EXPECT-ERROR: 9: fn-redefinition

class C {
    fn bump()
}

fn C::bump() { }
fn C::bump() { }
