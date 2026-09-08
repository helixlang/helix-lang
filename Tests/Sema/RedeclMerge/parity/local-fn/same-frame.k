// rule: a local `fn` lowers to a NON-CAPTURING LAMBDA bound to a name,
// so it follows the `var` rules, not the function rules. Two in one frame
// is a redefinition -- a name binds one value, so there is no overload set
// to form and no signature comparison to make. RedeclMerge never sees
// these: local decls are excluded from symbol tables by design.
// EXPECT-ERROR: 3: local-redefinition

fn outer() {
    fn helper(x: i32) -> i32 { return 1 }
    fn helper(x: i32) -> i32 { return 2 }
}
