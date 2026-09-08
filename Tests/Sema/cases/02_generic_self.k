// EXPECT: 0 errors. `self.items` binds to field (dependent record still looks up).
// EXPECT: `Self::origin()` step `::origin` binds; `Self` in type position canonical = Stack<T> (dependent).
class <T> Stack {
    var items: [T]

    fn Stack(self) { }

    fn push(self, v: T) {
        self.items.push(v)
    }

    fn twin(self) -> Self {
        return Self::origin()
    }

    static fn origin() -> Self {
        return Stack<T>()
    }
}

struct Point { var x: f64; var y: f64 }

extend Point {
    fn moved(self, dx: f64) -> Self {
        return Self { x: self.x + dx, y: self.y }
    }
}