// BEGIN-GENERATED-CHECKS (gen_checks.py) -- do not hand-edit, re-run instead
// RUN: { %kairo %s --type-check-only --print-ast=tree 2>&1 || true; } | %FileCheck %s --check-prefix=AST
// AST: ============== AST (tree) for fid 0 ==============
// AST-NEXT: TranslationUnit '{{.*}}'(fid=0)
// AST-NEXT: `-FunctionDecl  outer()
// AST-NEXT:   `-CompoundStmt (2)
// AST-NEXT:     |-DeclStmt
// AST-NEXT:     | `-FunctionDecl  helper()
// AST-NEXT:     |   `-CompoundStmt (0)
// AST-NEXT:     `-ConditionalStmt
// AST-NEXT:       |-cond
// AST-NEXT:       | `-BooleanLiteralExpr true
// AST-NEXT:       `-then
// AST-NEXT:         `-CompoundStmt (1)
// AST-NEXT:           `-DeclStmt
// AST-NEXT:             `-FunctionDecl  helper()
// AST-NEXT:               `-CompoundStmt (0)
// AST: -- 0 error(s), 0 warning(s) --
// END-GENERATED-CHECKS

fn outer() {
    fn helper() { }
    if true {
        fn helper() { }    // shadows the outer one, or collides?
    }
}
