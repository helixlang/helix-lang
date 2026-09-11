// RUN: { %kairo %s --type-check-only 2>&1 || true; } | %FileCheck %s --check-prefix=DIAG --allow-empty
// XFAIL: *
//
// WAITING ON: substituting a dependent DEFAULT template argument keeps
// qualifiers. `std::map<i32, i32>` defaults its allocator to
// `allocator<pair<const Key, T>>`; Kairo spells the filled default back to
// clang as `allocator<pair<int, int>>` -- the `const` is lost -- and clang's
// static_assert (map:934, "Allocator::value_type must be same type as
// value_type") rejects the specialization. The fill fails, `map` stays
// unfilled, and `.value_comp()` types from the pattern. When this XPASSes,
// replace the XFAIL with a SEMA check on the `[filled by clang` marker.
//
// DIAGNOSIS (2026-09-11): the const is lost on IMPORT, not export.
// TypeImport::_map maps `const int` through getCanonicalType() to the
// builtin i32; only references keep const (cxx_lref(T, is_const)). So
// `pair<const Key, T>` in the default argument imports as `pair<K, T>` and
// the registry keys the substituted instance as `pair<i32, i32>` --
// TypeExport has no qualifier left to re-add. The same loss in other
// positions: `f(const int*)` and `f(int*)` both import as `*i32` (a call
// is ambiguous where clang picks `f(int*)`), and `W<const int>` / `W<int>`
// collapse to one instance. Fixing it needs const in the canonical identity
// for non-reference types (CanonicalTypeStore identity, registry hash,
// _probe, is_dependent walks) or a side channel on template argument lists;
// a design decision, deliberately not taken here. Once done, also add
// std::set<i32> and std::unordered_map<i32, i32> fills.

ffi "c++" import "map"

fn cmp() -> bool {
    var m: std::map<i32, i32>
    var p: std::map<i32, i32>::value_type
    return m.value_comp()(p, p)
}

// DIAG-NOT: error
