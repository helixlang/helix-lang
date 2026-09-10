// RUN: { %kairo %s --type-check-only 2>&1 || true; } | %FileCheck %s --check-prefix=DIAG --allow-empty
// RUN: { %kairo %s --type-check-only --print-sema 2>/dev/null || true; } | %FileCheck %s --check-prefix=SEMA
//
// An instance of an imported template whose member's type is a DEPENDENT
// member typedef (`size_type` through `traits<A>`). The pattern cannot
// answer it; clang's instantiation can. The first lookup into `cont<i32>`
// fills the instance from clang (ForeignInstantiate), so `.size()` returns
// the canonical `unsigned long` -- u64 -- and the instance carries clang's
// layout.

ffi "c++" import "traits_typedef.h"

fn use_cont() -> u64 {
    var c: cont<i32>
    c.push(1)
    type my_size = traits<alloc>::size_type
    typeof my_size
    return c.size()
}

// No error at all is the size() assertion: the pattern's `size_type` is
// opaque and would not convert to the u64 return. `push(1)` binds a literal
// to the now-concrete `const int&` (OverloadResolution::_rank_literal).
// DIAG-NOT: error

// Both members come from the INSTANCE's table (the marker), with clang's
// layout: `T* first` + `unsigned long n` on the 64-bit test host.
// SEMA:      member  .push  {{.*}} -> fn push @ 13:5  [member of cont]  [filled by clang, 16 bytes, align 8]
// SEMA:      member  .size  {{.*}} -> fn size @ 12:5  [member of cont]  [filled by clang, 16 bytes, align 8]
// SEMA:      0 unknown, 0 errored
// SEMA:      0 of them dependent, 0 errored
