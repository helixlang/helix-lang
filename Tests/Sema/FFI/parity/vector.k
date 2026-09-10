// rule: a system C++ header imports like any other. `std::vector<i32>` is an
// instance of an imported class template (clang fills it), `push_back`
// takes `const T&`, `size` returns `size_t`.

ffi "c++" import "vector"

fn use_vector() -> usize {
    var v: std::vector<i32>
    v.push_back(1)
    return v.size()
}

// --- lit wiring -----------------------------------------------------------
// PARITY CASE. The .cpp beside this file is the ORACLE: the assertion is that
// kairo and clang reach the same verdict. See Tests/parity_check.py.
//
// Header roots are hard-passed by lit (--sysroot / --resource-dir, see
// lit.cfg.py). `<vector>` parses and indexes, `std::vector<i32>` resolves,
// and `push_back(const T&)` deduces T through the reference.
//
// XFAIL on the ONE thing left: `v.size()`. Its return type is vector's
// `size_type`, which libc++ spells through `allocator_traits<...>::size_type`
// -- a dependent member typedef. Answering it needs clang to complete the
// specialization (DependentResolution::member_of_foreign_instance, the
// stubbed half of S12), so today it imports opaque and does not convert to
// usize. When this XPASSes, remove the XFAIL.
//
// RUN: %parity %s
// REQUIRES: clang
// XFAIL: *
