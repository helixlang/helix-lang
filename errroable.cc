#include "libhelix/core/include/core.h"

question<int> foo(int a) {
    if ((a % 2) != 0) {
        return a + 1321;
    }

    return Error("error");
}

class FooError {  // see doesnt derive from Error
  public:
    explicit FooError(std::string message)
        : message(std::move(message)) {}

    std::string message;
};

question<int, FooError> boo(question<int> a) {
    if ((a % 2) != 0) {
        return a + 1321;
    }

    return FooError("error from boo");
}

int main() {
    question<int> a = null;

    a = 2;
    a = foo(a);  // errors

    try {  // prints "caught exception"
        std::cout << a << std::endl;
    } catch (const Error &e) { std::cout << "caught exception" << std::endl; }

    // if a?
    if (a.$inspect()) {  // prints "a is invalid"
        std::cout << "a is valid" << std::endl;
    } else {
        std::cout << "a is invalid" << std::endl;
    }

    if (a == null) {  // prints "a is invalid"
        std::cout << "a is invalid" << std::endl;
    } else if (a.$contains(Error())) {  // prints "a is invalid"
        std::cout << "a is of type Error" << std::endl;
    }

    a = boo(a);  // would trigger an throw
}

/* helix equivalent:
fn foo(a: int) -> int? {
    if a % 2 != 0 {
        return a + 1321;
    }

    panic Error("error");
}

fn main() -> i32 {
    let a: int? = null;

    a = 2;
    a = foo(a); // thing is since a is marked as an int? it will not panic, but if i was trying to
do let a: int = foo(a); it would panic

    try {
        print(a);
    } catch Error {
        print("caught exception");
    }

    if a? {
        print("a is valid");
    } else {
        print("a is invalid");
    }

    if a == null { // also null in a is valid
        print("a is invalid");
    } else if Error() in a {
        print("a is of type Error");
    }
}
*/

/*
cmd.exe /c "call "C:\Program Files (x86)\Microsoft Visual
Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1 && cl /Ox /TP /std:c++latest /EHsc
/nologo /Zc:__cplusplus  /W4 "Z:\devolopment\helix\helix-lang\errroable.cc"

*/