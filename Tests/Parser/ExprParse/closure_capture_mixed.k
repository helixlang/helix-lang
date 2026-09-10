// RUN: { %kairo %s --type-check-only --print-ast=tree 2>&1 || true; } | %FileCheck %s --check-prefix=AST
// XFAIL: *
//
// UNIMPLEMENTED: `&`-prefixed (by-reference) entries in a named capture list.
//
// Spec/Docs/closures.md:67 -- "### Mixed capture `|a, &b|` ... Unqualified
// names are captured by copy, `&`-prefixed names by reference", and the table
// at line 92-93 lists both `|&a, &b|` and `|a, &b|`.
//
// The parser accepts the copy capture `a` and then rejects `&b`, so the
// closure silently ends up with ONE capture where the source asked for two --
// a wrong AST, not just a diagnostic.
//
// Today (remove this block along with the XFAIL when by-ref captures land):
//   error [S047E]: expected capture name
//   ClosureExpr with a single `Capture a` child; `b` is dropped.
// AST: -- 0 error(s), 0 warning(s) --
// AST: Capture a
// AST-NEXT: Capture b
// AST-NOT: RecoveryExpr

fn Test() { var f = fn ()|a, &b| { }; }
