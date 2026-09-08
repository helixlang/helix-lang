// ported: clang/test/CXX/dcl.decl/dcl.meaning/dcl.fct.default/p4.cpp
// rule: the specified-once rule spans the in-class declaration and the
// out-of-line definition. f re-specifies (error); g ADDS a default to a
// parameter that lacked one (legal) -- the rule is per-parameter, not
// per-declaration.
// EXPECT-ERROR: 12: default-arg-redefinition

class C {
    fn f(i: i32 = 3)
    fn g(i: i32, j: i32 = 99)
}

fn C::f(i: i32 = 3) { }

fn C::g(i: i32 = 88, j: i32) { }
