// rule: a system C++ header imports like any other. `std::vector<i32>` is an
// instance of an imported class template (clang fills it), `push_back`
// takes `const T&`, `size` returns `size_t`.

ffi "c++" import "vector"

fn use_vector() -> u64 {
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
// `v.size()` is the case that needs clang's instantiation: vector's
// `size_type` is spelled through `allocator_traits<...>::size_type`, a
// dependent member typedef the pattern cannot answer. The first lookup into
// `std::vector<i32>` fills the instance from clang (ForeignInstantiate), so
// size() returns canonical `unsigned long` -- u64. The function returns u64
// to match: `usize` is a distinct type and crossing to it takes `as`
// (Tests/Sema/usize_distinct.k), exactly as the oracle's `unsigned long`.
//
// RUN: %parity %s
// REQUIRES: clang
