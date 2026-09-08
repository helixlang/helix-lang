// EXPECT: "unknown type name 'Nope'" once (T).
// EXPECT: "'i32' is a primitive type; it has no members" once (T).
// EXPECT: "'.' cannot follow a type; use '::'" once (ChainBinding).
// EXPECT: phase TypesResolved or Checked, 3 errors.
struct S { var a: i32 }

fn f() {
    var y: Nope
    var z: i32::Foo
    var q = S.a
}