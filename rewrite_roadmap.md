slowly start rewriting the following in helix itself:
- [ ] **Error Handling**
  - [ ] Custom error types
  - [ ] Error propagation
  - [ ] Error logging
  - [ ] Compile/runtime error distinction

- [ ] **AST Generation**
  - [ ] Node types
  - [ ] Tree traversal
  - [ ] Visitor pattern
  - [ ] Memory management

- [ ] **Code Generation**
  - [ ] C++ IR generation
  - [ ] LLVM IR generation
  - [ ] Code emission
  - [ ] Modularization/linking

- [ ] **Testing**
  - [ ] Unit tests
  - [ ] End-to-end tests
  - [ ] Benchmark tests
  - [ ] Test runner integration
  - [ ] Regression testing

- [ ] **Documentation**
  - [ ] Inline code comments
  - [ ] Automated documentation generation
  - [ ] Example-driven docs
  - [ ] API reference

- [ ] **Benchmarking**
  - [ ] Profiling
  - [ ] Performance benchmarks
  - [ ] Cross-platform comparison

- [ ] **Lexer**
  - [ ] Token definitions
  - [ ] Lexical analysis
  - [ ] Comment/whitespace handling

- [ ] **Parser**
  - [ ] Grammar rules
  - [ ] Recursive descent parser
  - [ ] Error recovery

- [ ] **Semantic Analysis**
  - [ ] Symbol table
  - [ ] Scope management
  - [ ] Declaration checks
  - [ ] Constant folding

- [ ] **Optimizer**
  - [ ] Constant propagation
  - [ ] Dead code elimination
  - [ ] Function inlining
  - [ ] Memory optimizations

- [ ] **AST Resolver**
  - [ ] Namespace resolution
  - [ ] Template instantiation
  - [ ] Member access

- [ ] **Borrow Checker**
  - [ ] Ownership rules
  - [ ] Lifetime analysis
  - [ ] Compile-time checks

- [ ] **Type Checker**
  - [ ] Type inference
  - [ ] Type constraints
  - [ ] Type coercion

- [ ] **Compiler RT**
  - [ ] Memory allocation
  - [ ] Core utilities
  - [ ] Runtime optimizations

- [ ] **Tooling**
  - [ ] Build system integration
  - [ ] Debugging tools
  - [ ] Linting
  - [ ] Code formatter

- [ ] **IDE Support**
  - [ ] Syntax highlighting
  - [ ] IntelliSense
  - [ ] Refactoring tools
  - [ ] Debugging support

- [ ] **LSP**
  - [ ] Definition provider
  - [ ] Hover provider
  - [ ] Code completion

- [ ] **JSON Generator**
  - [ ] AST to JSON
  - [ ] Pretty-print JSON

- [ ] **CST Generator**
  - [ ] CST node definitions
  - [ ] Pretty-printing CST

- [ ] **Preprocessor**
  - [ ] Macro handling
  - [ ] Conditional compilation
  - [ ] Include handling

- [ ] **Sys IO**
  - [ ] File handling
  - [ ] Network IO

- [ ] **CLI Parser**
  - [ ] Argument handling
  - [ ] Subcommand support
  - [ ] Config file integration
  
-- `sudo alternatives --config ld`: set the linker to use lld,
-- perl must be installed
-- zlib and zstd are optional

# Enhancing the Helix Compiler Toolchain

This document outlines the necessary modifications to the Helix compiler toolchain to support the Helix language. These modifications are categorized as follows:

- Lexer
- Parser
- Semantic Analysis
- Code Generation
- Driver
- Standard Library
- Tools

## Lexer
The lexer must be optimized for performance and designed to store file positions for tokens separately. The proposed structure includes distinct classes for `Location`, `Token`, and `TokenKind`, ensuring clarity and efficiency.
helix-lang\libs\llvm-18.1.9-src\clang\include\clang\Basic\CharInfo.h

### Suggested Structures

#### Location
```rs
class Location {
    let file: *File;
    let start: usize;
    let end: usize;

    fn Location(self, file: *File, start: usize, end: usize);
    
    fn value(self) -> string?;
    fn line(self) -> usize;
    fn column(self) -> usize;
    fn serialize(self) -> map::<string, string>;

    op == fn (self, other: Location) -> bool;
    op != fn (self, other: Location) -> bool;
    op < fn (self, other: Location) -> bool;
    op > fn (self, other: Location) -> bool;
    op <= fn (self, other: Location) -> bool;
    op >= fn (self, other: Location) -> bool;
}
```

#### Token
```rs
class Token {
    let value: string;
    let kind: TokenKind;
    let location: Location?;

    enum RemapMode {
        Location,
        Value,
        Kind,
        KindAndValue,
        LocationAndKind,
        ValueAndLocation,
        All,
    }

    fn Token(self, kind: TokenKind, value: string, location: Location?);
    
    fn remap(self, token: &Token, mode: RemapMode) -> bool;
    fn location(self) -> Location; // Error if location is null
    fn value(self) -> string?;
    fn serialize(self) -> map::<string, string>;

    op == fn (self, other: Token) -> bool;
    op != fn (self, other: Token) -> bool;
    op == fn (self, other: TokenKind) -> bool;
    op != fn (self, other: TokenKind) -> bool;
    op == fn (self, other: string) -> bool;
    op != fn (self, other: string) -> bool;
}
```

#### TokenKind
```rs
class TokenKind {
    enum Kind {
        // Token types
    }

    let kind: Kind;

    fn TokenKind(self, kind: Kind);
    fn TokenKind(self, value: string);

    fn to_string(self, kind: Kind?) -> string?;
    fn to_kind(self, value: string?) -> Kind?;
    fn serialize(self) -> map::<string, string>;

    op == fn (self, other: TokenKind) -> bool;
    op != fn (self, other: TokenKind) -> bool;
    op == fn (self, other: string) -> bool;
    op != fn (self, other: string) -> bool;
    op == fn (self, other: Kind) -> bool;
    op != fn (self, other: Kind) -> bool;

    op as fn (self) -> string;
    op as fn (self) -> Kind;
}
```

## Driver
The driver will consolidate shared logic between the compiler and the toolchain. Files in this module will compile into binaries, encapsulating essential functionality for the compiler and tools.

### File Class
```rs
class File {
    let abs_path: string;
    let content: string;

    fn File(self, abs_path: string, content: string);
    fn serialize(self) -> map::<string, string>;
    fn read(self) -> string;
    op [] fn (self, index: Range::<usize>) -> string?;

    op == fn (self, other: File) -> bool;
    op != fn (self, other: File) -> bool;
}
```

### Memory-Mapped File
```rs
class MemoryMappedFile;
```

### Compiler CLI Options
The compiler (helix) CLI will adopt the following options for flexibility and clarity:

| Option                | Description                                                                                                                                   |
|-----------------------|-----------------------------------------------------------------------------------------------------------------------------------------------|
| `-h`, `--help`            | Displays a detailed help message with usage instructions.                                                                                 |
| `-v`, `--version`         | Prints the current version of the compiler.                                                                                               |
| `-o`, `--output`          | Specifies the directory for compiler output files.                                                                                        |
| `-lib`, `--library`       | Builds either a Helix vile format or a static/dynamic library depending on flags.                                                         |
| `-static`, `--static`     | Generates a static binary or library for platform-specific use.                                                                           |
| `-dyn`, `--dynamic`       | Produces a dynamically linked binary or library.                                                                                          |
| `-i/I`, `--include`       | Adds a directory to the list of include paths for header files.                                                                           |
| `-L`, `--include-library` | Adds a library directory to the search paths for linking.                                                                                 |
| `-l`, `--link`            | Links a specified library to the output binary. Syntax: `-l library_name`.                                                                |
| `-O`, `--optimize`        | Sets the optimization level for the output (0 for none, 5 for maximum).                                                                   |
| `-d`, `--debug`           | Enables debug symbols in the compiled binary.                                                                                             |
| `-cfg`, `--config`        | Provides a `helix.toml` configuration file for custom compiler settings.                                                                  |
| `-m/M`, `--macro`         | Injects a macro definition into the compilation process. Syntax: `-M macro_name "macro_value"`.                                           |
| `-E`, `--error`           | Configures error reporting verbosity. Options: `none`, `warning`, `info`, `debug`, `all`, `hard`.                                         |
| `-ffi, --ffi`             | Specify the foreign function interface type (e.g., `-ffi c` or `--ffi c` for C headers).                                                  |
| `-t, --target`            | Define the target ABI in the format `arch-vendor-os` (e.g., `-t x86_64-linux-gnu` or `--target x86_64-linux-gnu`).                        |
| `-test, --test`           | Compile test blocks into a binary (e.g., `helix -o app.test.exe -test main.hlx` or `--test main.hlx`).                                    |
| `-fI, --force-include`    | Force include a file into the compilation process, regardless of its extension or import status.                                          |
| `-x, --ext`               | Specify file extension(s) for compilation (e.g., `-x hlx` or `--ext hlx`). Supported extensions: `hlx`, `helix`, `hdlx`, `c`, `cpp`, etc. |
| `--ffi-include`           | Add a directory to the FFI generator library search path. *(No short flag for clarity)*                                                   |
| `--emit`                  | Outputs intermediate results like `tokens`, `ast`, `ir`, `llvm`, `asm`, or `cxx`.                                                         |
| `--verbose`               | Enables detailed logging of the compilation process.                                                                                      |
| `--quiet`                 | Suppresses all compiler output except for errors.                                                                                         |
| `--suppress`              | Outputs only to stderr, leaving stdout empty.                                                                                             |
| `--cxx-flags`             | Passes additional flags to the underlying C++ compiler. Syntax: `--cxx-flags "-flag1 -flag2"`.                                            |
| `--ld-flags`              | Provides linker-specific flags for custom linking behavior. Syntax: `--ld-flags "-flag1 -flag2"`.                                         |

**Format/Compiler Feature Flags**

| **Option**                      | **Description**                                                |
|---------------------------------|----------------------------------------------------------------|
| `--f --style <style>`           | Sets output style: (`msvc`, `gcc`, `clang`, `helix`, `json`)   |
| `--f [-no] -ansi`               | Toggles ANSI escape sequences in output.                       |
| `--f [-no] -color`              | Toggles colored output.                                        |
| `--f [-no] -utf8`               | Enables or disables UTF-8 encoding support.                    |
| `--f [-no] -unicode`            | Enables or disables Unicode support.                           |
| `--f [-no] -line-number`        | Toggles line numbers in error messages.                        |
| `--f [-no] -file-name`          | Toggles file names in error messages.                          |
| `--f [-no] -timestamp`          | Toggles timestamps in logs.                                    |
| `--f [-no] -quiet-errors`       | Suppresses non-critical errors.                                |
| `--f [-no] -warnings-as-errors` | Treats warnings as errors.                                     |
| `--f [-no] -verbose-errors`     | Enables detailed error messages.                               |
| `--f [-no] -pretty-print`       | Toggles human-readable output formatting.                      |

**Compilation Rules**

| Option                   | Description                                                               |
|--------------------------|---------------------------------------------------------------------------|
| `--c [-no] -implicit`    | Disallows/allows implicit type conversions.                               |
| `--c [-no] -unsafe`      | Enables/disables unsafe code segments.                                    |
| `--c [-no] -ffi`         | Enables/disables support for Foreign Function Interfaces (FFI).           |
| `--c [-no] -semicolon`   | Configures whether semicolons are mandatory or optional (auto-detected).  |



| **Option**                 | **Description**                                                          |
|----------------------------|--------------------------------------------------------------------------|
| `--c [-no] -implicit`      | Toggles implicit type conversions.                                       |
| `--c [-no] -unsafe`        | Enables or disables unsafe code.                                         |
| `--c [-no] -ffi`           | Toggles FFI support.                                                     |
| `--c [-no] -semicolon`     | Configures semicolon requirements.                                       |
| `--c [-no] -const-checks`  | Toggles strict `const` enforcement.                                      |
| `--c [-no] -overflow`      | Toggles overflow checks.                                                 |
| `--c [-no] -strict-null`   | Treats null references as errors.                                        |
| `--c [-no] -inline`        | Toggles aggressive inlining.                                             |
| `--c [-no] -tail-call`     | Toggles tail-call optimization.                                          |
| `--c [-no] -exceptions`    | Toggles exception handling.                                              |
| `--c [-no] -yield`         | Toggles `yield` support.                                                 |
| `--c [-no] -coerce`        | Toggles type coercion.                                                   |
| `--c [-no] -static-checks` | Toggles additional static checks.                                        |
| `--c [-no] -dead-code`     | Toggles dead code elimination.                                           |
| `--c [-no] -export`        | Toggles exporting global symbols.                                        |
| `--c [-no] -mutable`       | Toggles mutable variables. (if `-no` all variables are always immutable) |
| `--c [-no] -legacy`        | Toggles legacy feature support.                                          |

## Parser
Implement a **LALR(1)** parser to handle ambiguous grammars and improve error recovery.
