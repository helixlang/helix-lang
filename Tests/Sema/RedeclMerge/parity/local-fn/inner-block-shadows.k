// rule: an inner block SHADOWS rather than colliding, same as a `var`.
// Visible in --print-sema as a shadow note on the declare row; not a
// diagnostic, matching clang, which only says so under -Wshadow.
// EXPECT-CLEAN
// EXPECT-SEMA: inner helper shadows fn helper @ 2:5 in an enclosing frame

fn outer() {
    fn helper() { }
    if true {
        fn helper() { }
    }
}
