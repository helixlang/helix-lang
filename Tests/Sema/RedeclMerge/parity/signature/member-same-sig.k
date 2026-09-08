// rule: a same-signature pair inside a type body merges exactly as at
// file scope. Exercises _merge_in recursion through a type DeclContext.
// EXPECT-ERROR: 8: fn-redefinition

class C {
    fn bump(x: i32) { }
    fn bump(x: i32) { }
}
