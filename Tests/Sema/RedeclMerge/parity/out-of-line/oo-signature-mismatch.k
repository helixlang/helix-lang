// ported: clang/test/SemaCXX/out-of-line-def-mismatch.cpp
// rule: an out-of-line definition must match an in-class SIGNATURE, not
// merely a name. N(a) only checked the name, so this is the case R(b)
// exists to catch.
// EXPECT-ERROR: 7: out-of-line-signature-mismatch

class C {
    fn bump()
}

fn C::bump(x: i32) { }
