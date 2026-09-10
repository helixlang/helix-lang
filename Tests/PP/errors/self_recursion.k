// RUN: { %kairo %s --type-check-only --cmax-macro-expansions=200 2>&1 || true; } | %FileCheck %s --check-prefix=DIAG
// XFAIL: *
//
// BUG: direct self-recursion is not detected as CYCLIC. It is caught only by
// running out of expansion budget.
//
// PPWalker::_expand has the right guard -- it walks `inv.chain` looking for
// the macro's own symbol and reports "cyclic expansion of macro 'boom!'" --
// but for a body that IS the invocation the parent splice frame is already
// exhausted and popped by the time the nested invocation is detected, so the
// chain arrives EMPTY and the check never fires. TokenSource.k documents this
// exact hazard on `SpliceFrame::chain`.
//
// The user-visible cost is not just a worse message. The default budget is
// 65536 expansions at roughly a millisecond each, so `macro boom!() { boom!() }`
// occupies the compiler for well over a minute before it says anything. The
// budget cap in the RUN line above is what keeps this test fast; remove it
// when the cycle check works, since it will no longer be needed.
//
// Today, with the cap: "macro expansion budget exhausted".
// DIAG: error [R008E]: cyclic expansion of macro 'boom!'
// DIAG: recursion occurs here, in the macro body

macro boom!() { boom!() }
var a = boom!();
