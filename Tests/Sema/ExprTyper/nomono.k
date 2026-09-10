// RUN: { %kairo %s --type-check-only 2>&1 || true; } | %FileCheck %s --check-prefix=DIAG --allow-empty
// RUN: { %kairo %s --type-check-only --print-sema 2>/dev/null || true; } | %FileCheck %s --check-prefix=SEMA
//
// The definition side of a generic. NOTHING here is monomorphized: this pass
// types the TEMPLATE, and every question whose answer depends on the
// arguments is deferred to M2 rather than guessed or diagnosed.
//
// Two different answers appear in the dump and they are not the same thing:
//
//   `T`         the expression HAS a type, and that type is dependent.
//               `x`, `id(x)`, `h->get()` are all of type T here, which is
//               everything a definition-time reader can be told.
//   `<unknown>` the answer needs the instantiation. `x + x` is the case:
//               which `op +` applies to T is chosen by the argument, so
//               there is no operator table to consult yet -- and REJECTING
//               it here would make every generic that adds ill-formed.
//
// The invariant this file protects: a generic body produces NO diagnostics.

class <T> Holder {
    var v: T
    fn Holder(self) { }
    fn get(self) -> T { return self.v }
}

fn <T> id(x: T) -> T { return x }

fn <T> body(x: T, h: *Holder<T>) {
    var a = x;
    var b = id(x);
    var c = h->get();
    var d = x + x;     // deferred, not rejected
    var e = h->v;
    var f = &x;        // `*T` whatever T is: no instantiation needed
}

// DIAG-NOT: error
// DIAG-NOT: warning

// SEMA: -- expression types --
// SEMA:      BinaryOperatorExpr {{.*}} : <unknown>

// SEMA: -- inferred decls --
// SEMA:      a : T
// SEMA-NEXT: b : T
// SEMA-NEXT: c : T
// SEMA-NEXT: e : T
// SEMA-NEXT: f : *T
