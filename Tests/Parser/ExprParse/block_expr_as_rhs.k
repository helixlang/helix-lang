// RUN: { %kairo %s --type-check-only --print-ast=tree 2>&1 || true; } | %FileCheck %s --check-prefix=AST
// XFAIL: *
//
// UNIMPLEMENTED: a BLOCK used as an expression on the right-hand side of a
// binding.
//
// Spec/Docs/control-flow.md:561 -- "Blocks are expressions when used as the
// right-hand side of a binding. The last expression in the block is [the
// value]." The parser instead commits to a SET LITERAL the moment it sees `{`
// in expression position, so `{ var t = 1; t * 2 }` is parsed as `{ <expr> }`
// and falls apart at the `var`.
//
// Today (remove this block along with the XFAIL when block-exprs land):
//   error [S054E]: expected '}' to close set literal
//   error [S052E]: expected expression, found 'var'
//   error [S052E]: expected expression, found '}'
//
// The contract asserted below is the minimum: it parses, with no errors, and
// the initializer is not mistaken for a set literal.
// AST: -- 0 error(s), 0 warning(s) --
// AST-NOT: SetLiteralExpr
// AST-NOT: RecoveryExpr

fn Test() { var x = { var t = 1; t * 2 }; }
