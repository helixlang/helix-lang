// EXPECT: "use of undeclared identifier 'nope'" once.
// EXPECT: "'Self' used outside of a type body" once (expression position).
// EXPECT: phase NamesResolved, 2 errors.
fn f() {
    var x = nope
    var w = Self::a
}