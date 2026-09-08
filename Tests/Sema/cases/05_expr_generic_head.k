// EXPECT: 0 errors. step `::make` after `Box<i32>` head binds (Type anchor via registry).
// EXPECT: `Box<i32>::make` and `Box<f64>::make` are two RecordTypes (types section count).
class <T> Box {
    var v: T
    static fn make() -> Box<T> { return Box<T>() }
    fn Box(self) { }
}

fn use() {
    var a = Box<i32>::make()
    var b = Box<f64>::make()
}