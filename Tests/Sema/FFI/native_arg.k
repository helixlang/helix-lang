// RUN: { %kairo %s --type-check-only 2>&1 || true; } | %FileCheck %s --check-prefix=DIAG --allow-empty
// RUN: { %kairo %s --type-check-only --print-sema 2>/dev/null || true; } | %FileCheck %s --check-prefix=SEMA
//
// The phase-A limit, asserted as it stands. `cont<Point>` with a KAIRO
// struct as the argument cannot be filled: clang's AST has never heard of
// `Point`, and there is no C++ forward shape to give it yet
// (TypeExport: TODO(ffi)). The fill fails once, cleanly -- no ICE, no
// diagnostic -- and lookup falls back to the pattern: `push(const T&)`
// still substitutes T, while `size()`'s dependent `size_type` stays
// unanswered. When Kairo types can be exported into phase A, this test
// changes to look like traits_typedef.k.

ffi "c++" import "traits_typedef.h"

struct Point {
    var x: i32
    var y: i32
}

fn use_native() {
    var c: cont<Point>
    var p: Point
    c.push(p)
    var n = c.size()
}

// DIAG-NOT: error

// Answered from the PATTERN: no fill marker on either row, and size()'s
// dependent `size_type` comes back opaque.
// SEMA:      member  .push  {{.*}} -> fn push @ 14:5  [member of cont]{{$}}
// SEMA:      member  .size  {{.*}} -> fn size @ 13:5  [member of cont]{{$}}
// SEMA:      0 unknown, 0 errored
// SEMA:      -- inferred decls --
// SEMA-NEXT: n : <ffi type>
