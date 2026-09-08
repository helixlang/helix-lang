// EXPECT: 0 errors. `b.bits` binds (member of the EXPLICIT spec, not the primary).
// EXPECT: `b.describe()` binds via the primary's extension `extend <T> Box<T>`.
// EXPECT: `c.items` binds through the implicit instantiation -> pattern walk.
class <T> Box {
    var items: [T]
}

class <> Box<bool> {
    var bits: u8
}

extend <T> Box<T> {
    fn describe(self) -> i32 = 0
}

fn use() {
    var b: Box<bool>
    var c: Box<i32>
    var k = b.bits
    var d = b.describe()
    var e = c.items
}