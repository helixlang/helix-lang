// EXPECT: exactly one error: "type alias 'A' is defined in terms of itself" with B as a note.
// EXPECT: `Meters` canonical == f64; `Mat<i32>` marked instantiation-dependent, no error.
// EXPECT: `M::Inner` via alias `M::X` resolves inside module M.
type Meters = f64
type <T> Mat = [[T]]

type A = B
type B = A

module M {
    struct Inner { var q: i32 }
    type X = Inner
}

fn use(m: Meters, g: Mat<i32>, i: M::X) { }