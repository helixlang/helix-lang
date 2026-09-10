#!/usr/bin/env python3
"""One parity case: kairo on the .k, clang on the .cpp beside it, same verdict.

The .cpp is the ORACLE (Tests/Sema/RedeclMerge/README.md, OVERLOAD.md: kairo's
behaviour here is 1:1 with C++). A divergence is a kairo bug unless it is
listed in kairo-only/README as a deliberate divergence -- in which case the
case does not belong in parity/ at all.

Only the VERDICT is compared, accept vs reject, not the message: the two
compilers have no reason to word a diagnostic the same way, and pinning kairo's
text to clang's would be a test of translation rather than of behaviour. What
rule each side applied is printed on divergence so a human can judge it.

Usage: parity_check.py <kairo> <clang> <file.k>
       <kairo> may carry flags as one shell-quoted word ('kairo --sysroot=..'):
       lit passes the C++ header roots that way.
Exit:  0 = same verdict, 1 = diverged, 2 = harness problem (missing oracle).
"""
import os
import shlex
import subprocess
import sys

TIMEOUT = 120


def verdict(argv):
    """(accept|reject, combined output). A crash is NOT a reject."""
    try:
        p = subprocess.run(argv, capture_output=True, text=True, timeout=TIMEOUT)
    except subprocess.TimeoutExpired:
        return "timeout", "timed out after %ds" % TIMEOUT
    out = (p.stdout or "") + (p.stderr or "")
    # A signal (negative returncode) means it died, which is neither an accept
    # nor a reject -- surfacing it as "reject" would let a crash masquerade as
    # correct behaviour on every reject case in the corpus.
    if p.returncode < 0:
        return "crash(signal %d)" % -p.returncode, out
    return ("accept" if p.returncode == 0 else "reject"), out


def main():
    if len(sys.argv) != 4:
        print(__doc__, file=sys.stderr)
        return 2
    kairo, clang, kfile = sys.argv[1:4]

    cpp = kfile[:-2] + ".cpp" if kfile.endswith(".k") else kfile + ".cpp"
    if not os.path.isfile(cpp):
        print("NO-ORACLE: %s has no .cpp beside it.\n"
              "  Every case under parity/ is defined by its oracle. If C++ has\n"
              "  no analog for this construct it belongs in kairo-only/, with a\n"
              "  file-level comment saying why." % os.path.basename(kfile),
              file=sys.stderr)
        return 2

    kv, kout = verdict(shlex.split(kairo) + [kfile, "--type-check-only"])
    cv, cout = verdict([clang, "-fsyntax-only", "-std=c++20", cpp])

    if kv == cv:
        print("ok  %s  (both %s)" % (os.path.basename(kfile), kv))
        return 0

    print("PARITY DIVERGENCE: %s" % kfile, file=sys.stderr)
    print("  kairo: %-8s clang: %s" % (kv, cv), file=sys.stderr)
    print("  --- kairo ---", file=sys.stderr)
    print(kout.rstrip() or "    (silent)", file=sys.stderr)
    print("  --- clang (oracle) ---", file=sys.stderr)
    print(cout.rstrip() or "    (silent)", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
