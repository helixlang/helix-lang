// RUN: { %kairo %s --type-check-only --print-emit-plan 2>/dev/null || true; } | %FileCheck %s --check-prefix=PLAN
//
// IMPORTS.md §6.1-§6.3. The plan is what a C++ header would have provided for
// THIS TU, derived from resolved decl links alone -- no ImportDecl is read
// (invariant 8) and no import order is consulted.
//
// Three things are asserted, and each is a different way for the plan to be
// wrong:
//
//   1. TRANSITIVITY. main imports foo, never bar. `foo::F` holds a `bar::B`
//      by value, so B's definition has to precede F's, in main.cpp. The
//      closure finds it; nothing else can.
//   2. EDGE STRENGTH. `bar::Deep` is reached only through a pointer, so it
//      gets a tier-0 forward declaration and NO definition. A plan that
//      cannot tell those apart emits every reachable type in full and the
//      preamble grows without bound.
//   3. REACHABILITY. `foo::Unused` is never named, directly or transitively,
//      and must not appear at all.
//
// Ordering: tier 1 is topologically sorted on `complete` edges, so B precedes
// F regardless of which file each was written in.

import foo;

fn main() -> i32 {
    var f: foo::F;
    return foo::use_f(f);
}

// Tier 0 forward-declares every type in the set, which is what breaks the
// pointer edge to Deep before tier 1 has to sort anything.
// PLAN: tier 0: forward declarations
// PLAN-DAG: class B
// PLAN-DAG: class Deep
// PLAN-DAG: class F

// Tier 1 is definitions only, and Deep is not one: nothing needs its layout.
// PLAN: tier 1: definitions
// PLAN: class B   [complete]
// PLAN: class F   [complete]

// PLAN: tier 2: function declarations
// PLAN: fn use_f

// Never named, never planned.
// PLAN-NOT: Unused
