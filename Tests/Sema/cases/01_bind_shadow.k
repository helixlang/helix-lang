// EXPECT: 0 errors. `x` in inner block shadows param x (trace shows shadows).
// EXPECT: two `for var i` loops -> no redefinition.
// EXPECT: `helper` and `Local` bind (statement-scope decls), `Local` resolves as a type.
var g: i32 = 1

fn f(x: i32) -> i32 {
    fn helper(y: i32) -> i32 = y + 1

    class Local {
        var next: *Local
        var v: i32
    }

    var l: Local
    l.v = helper(x)

    for var i = 0; i < 3; i++ { g += i }
    for var i = 0; i < 3; i++ { g -= i }

    {
        var x = 10
        g += x
    }
    return x + g
}