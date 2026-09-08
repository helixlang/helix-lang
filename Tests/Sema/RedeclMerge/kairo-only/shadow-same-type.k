// NO ORACLE: Rust does not lint shadowing at all; C++ -Wshadow fires on
// every shadow regardless of type and is off by default for that reason.
// Kairo warns only on SAME-TYPE shadowing, which is the case that means
// you probably wanted an assignment.
//
// EXPECT-WARN: 4: shadow-same-type
// EXPECT-CLEAN otherwise

fn f(cond: bool) {
    var count: i32 = 0
    if cond {
        var count: i32 = 1
    }
}

fn g(cond: bool) {
    const raw: i32 = 1
    if cond {
        const raw: bool = true
    }
}