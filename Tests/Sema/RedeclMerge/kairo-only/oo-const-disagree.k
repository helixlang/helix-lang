// NO ORACLE: C++ puts the cv-qualifier of the implicit object parameter
// in the SIGNATURE, so there the in-class const decl and the non-const
// out-of-line def are two different functions and the def matches nothing.
// Kairo forbids const overloading, so they are ONE function whose two
// links disagree about const -- which is what must be reported.
//
// This is the case that proves _same_signature ignores const for
// Kairo-authored decls: it must MATCH and then disagree, not fail to
// match and report "does not match any declaration".
//
// EXPECT-ERROR: 5: modifier-disagreement (const)

class Bar {
    fn foo(self) const -> i32
}

fn Bar::foo(self) -> i32 { return 1 }
