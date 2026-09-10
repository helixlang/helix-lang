// NO ORACLE: see const-rebind.k for the rule and why neither C++ nor Rust
// settles it.
// EXPECT-ERROR: 4: local-redefinition
// EXPECT-ERROR: 9: local-redefinition
// EXPECT-ERROR: 14: local-redefinition

fn var_twice() {
    var x = 10
    var x = 12
}

fn const_then_var() {
    const y = 10
    var y = 12
}

fn var_then_const() {
    var z = 10
    const z = 12
}

// --- lit wiring -----------------------------------------------------------
// Kept at the BOTTOM on purpose: the EXPECT-ERROR annotation above names an
// absolute LINE NUMBER in the code, so anything inserted above the code
// silently invalidates it.

// --- lit wiring -----------------------------------------------------------
// Kept at the BOTTOM on purpose: the EXPECT-ERROR annotation above names an
// absolute LINE NUMBER in the code, so anything inserted above the code
// silently invalidates it. Do not let gen_checks.py move it back up.
// BEGIN-GENERATED-CHECKS (gen_checks.py) -- do not hand-edit, re-run instead
// RUN: { %kairo %s --type-check-only --print-ast=tree 2>&1 || true; } | %FileCheck %s --check-prefix=AST
// AST: ============== AST (tree) for fid 0 ==============
// AST-NEXT: TranslationUnit '{{.*}}'(fid=0)
// AST-NEXT: |-FunctionDecl  var_twice()
// AST-NEXT: | `-CompoundStmt (2)
// AST-NEXT: |   |-DeclStmt
// AST-NEXT: |   | `-VariableDecl var x
// AST-NEXT: |   |   `-IntegerLiteralExpr 10
// AST-NEXT: |   `-DeclStmt
// AST-NEXT: |     `-VariableDecl var x
// AST-NEXT: |       `-IntegerLiteralExpr 12
// AST-NEXT: |-FunctionDecl  const_then_var()
// AST-NEXT: | `-CompoundStmt (2)
// AST-NEXT: |   |-DeclStmt
// AST-NEXT: |   | `-VariableDecl const y
// AST-NEXT: |   |   `-IntegerLiteralExpr 10
// AST-NEXT: |   `-DeclStmt
// AST-NEXT: |     `-VariableDecl var y
// AST-NEXT: |       `-IntegerLiteralExpr 12
// AST-NEXT: `-FunctionDecl  var_then_const()
// AST-NEXT:   `-CompoundStmt (2)
// AST-NEXT:     |-DeclStmt
// AST-NEXT:     | `-VariableDecl var z
// AST-NEXT:     |   `-IntegerLiteralExpr 10
// AST-NEXT:     `-DeclStmt
// AST-NEXT:       `-VariableDecl const z
// AST-NEXT:         `-IntegerLiteralExpr 12
// AST: -- 0 error(s), 0 warning(s) --
//
// -- diagnostics (order matches emission order) --
// AST: {{.*}}: error [R029E]:
// AST: {{.*}}: error [R029E]:
// AST: {{.*}}: error [R029E]:
// END-GENERATED-CHECKS
