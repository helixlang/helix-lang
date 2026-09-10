// RUN: { %kairo %s --type-check-only --print-ast=tree 2>&1 || true; } | %FileCheck %s --check-prefix=AST
// XFAIL: *
//
// UNIMPLEMENTED: `|&|`, capture-all-by-reference.
//
// Spec/Docs/closures.md:51 -- "### Capture by reference `|&|` ... To capture
// all variables by reference, append `|&|` after the parameter list", and the
// summary table at line 90 lists `|&|` as "All captures by reference".
//
// The parser's own message says "expected capture name or '*'", so the
// implementation spells capture-all `*` while the docs spell it `&`. That
// divergence needs deciding before this test can pass -- if `*` is the
// intended spelling, the DOCS are what is wrong here, not the parser.
//
// Today (remove this block along with the XFAIL once the two agree):
//   error [S047E]: expected capture name or '*' in capture list
// and the ClosureExpr comes out with no Capture children at all.
// AST: -- 0 error(s), 0 warning(s) --
// AST: ClosureExpr
// AST-NOT: RecoveryExpr

fn Test() { var f = fn ()|&| { }; }
