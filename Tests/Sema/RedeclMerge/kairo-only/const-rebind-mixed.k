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
