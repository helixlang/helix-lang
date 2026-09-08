// NO ORACLE: C++ has no ! return type. [[noreturn]] is an attribute, not
// part of the function type, so clang has no verdict to copy.
//
// UNDECIDED -- fill in the expectation once OVERLOAD.md rules on it:
//   (a) ! is a distinct return type   -> this is a return-type conflict
//   (b) ! is a modifier on void       -> these merge silently, which is
//                                        almost certainly wrong
// Note is_noreturn is a separate bool on FunctionDecl and return_type may
// be null for !, in which case _return_canonical maps BOTH to void and
// this file passes for the wrong reason. Check that before trusting it.
//
// EXPECT-UNDECIDED

fn n(i32) -> !
fn n(x: i32) { }
