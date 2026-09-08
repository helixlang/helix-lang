// EXPECT: 0 errors. `v.total()` binds via `extend <T> [T]` (shape-keyed).
// EXPECT: `n.twice()` binds via `extend i32` (exact canonical).
extend <T> [T] {
    fn total(self) -> i32 = 0
}

extend i32 {
    fn twice(self) -> i32 = self * 2
}

fn use(v: [i32], n: i32) {
    var a = v.total()
    var b = n.twice()
}