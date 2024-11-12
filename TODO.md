# Todo list of all the things remaining to be done in the short term. (high to low priority)

- [x] Parse TurboFish syntax (temporary until the symbol table is implemented)
- [x] Parse and codegen reference and pointer types along with nullability
- [ ] Parse and codegen function pointers
- [x] Parse Tuple types and codegen them
- [ ] Add support for packing and unpacking functions and destructuring
- [ ] Parse simple macros and invocations
- [ ] Parse eval if statements
- [ ] Get CXIR compile messages to convert to helix errors
- [x] Parse basic helix imports (no symbol resolution)
- [ ] Codegen C++ headers to allow C++ to call helix code
- [ ] Parse and codegen catch blocks with no catch type
- [ ] Make test syntax work for definitions and usages
- [ ] Make classes immutable by default
- [x] Get f-strings working
- [x] Convert clang errors into the helix error msg format.
- [ ] Convert gcc errors into the helix error msg format.
- [x] Convert msvc errors into the helix error msg format.
- [ ] Fix ast error messages, where the message tells a fix to also add a quick fix to the error
- [x] Add support for global scopes in the parser
- [ ] Add panic unwinding support
- [ ] Parse and add extend support for generic specializations

# Completed:
### Parser:
- [x] Parse types
- [x] Parse functions
- [x] Parse structs
- [x] Parse enums
- [x] Parse constants
- [x] Parse variables
- [x] Parse Operator Overloads
- [x] Parse Interfaces

# draft: **Merging Pointers and References**
### Rule Set (safe pointers):
- **`*i32`:**
  - Behaves like a reference: accessed without explicit dereferencing.
  - Can be uninitialized initially but must be initialized before usage. (checked at compile-time)
  - Must always point to a valid memory address. (enforced by not being allowed to set a custom location to point to)
  - Cannot be set to `&null`.
  - Internally auto-dereferences during code generation.

```rs
let a: i32 = 123;
let a_ptr: *i32 = &a;

print(a_ptr); // prints 123
a_ptr + 2;    // a_ptr = 125
```

- **`*i32?`:**
  - Similar to a reference: accessed without explicit dereferencing.
  - Can be uninitialized initially but must be initialized before usage. (checked at compile-time)
  - Can point to `&null`, and this can be checked using `... != &null`. (compile-time warning if not checked for null)
  - Must always point to a valid memory address. (enforced by not being allowed to set a custom location to point to)
  - Internally auto-dereferences during code generation.

```rs
let a: i32 = 123;
let a_ptr: *i32? = &null;

a_ptr = &a; // a_ptr = 123

if a_ptr != &null {
    print(a_ptr); // prints 123
    a_ptr + 2;    // a_ptr = 125
}
```

### Rule Set (unsafe pointers):
- **`unsafe *i32`:**
  - Requires explicit dereferencing for access.
  - Must be initialized before usage. (checked at compile-time)
  - Can be set to `&null` and does not need to point to a valid memory address.

```rs
let a: i32 = 123;
let a_ptr: unsafe *i32 = 0xAB12 as unsafe *i32; // a_ptr points to 0xAB12

print(*a_ptr); // if 0xAB12 is uninitialized then segfault or undefined behavior, else reads data in 0xAB12

a_ptr = &a; // reassign to a address
```

- **`unsafe *i32?`:**
  - Requires explicit dereferencing for access.
  - Doesn’t need to be initialized by the time of usage.
  - Can be set to `&null` and does not need to point to a valid memory address.

```rs
let a: i32 = 123;
let a_ptr: unsafe *i32?; // a_ptr points to 0xAB12

a_ptr = 0xAB12 as *i32;

print(*a_ptr); // if 0xAB12 is uninitialized then segfault or undefined behavior, else reads data in 0xAB12

a_ptr = &a; // reassign to a address
```

- **`&i32?` | `&i32`:**
  - Should be disallowed completely.
  - If a user needs a version of `*i32` that cannot be uninitialized, they should use `reference<i32>` instead.