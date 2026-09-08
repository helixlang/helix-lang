// Not a language rule: a REGRESSION for the driver. Running sema twice
// over one TU must be a no-op the second time. This shape produced a
// bogus [overload set x3] when the same path was passed twice on one
// command line (source manager interns by path -> same fid -> sema ran
// again over an already-Checked TU, and decls were re-inserted).
//
// Growth was triangular (1, 3, 6, 10: run N appended N), so a two-run
// assertion is enough to catch a regression.
//
// EXPECT-CLEAN
// EXPECT-SEMA: <tu> declares one and only one, exactly once
// RUN TWICE: pass this path twice in one invocation; both dumps identical

fn one() {
    var x = 1
    var y = 2
    var z = 3
}
