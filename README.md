# Roman Urdu Compiler

Roman Urdu Compiler is a small educational programming language implemented for the Compiler Construction course project. The project has one shared Flex scanner, one shared Bison grammar, one semantic checker, and one command-line program with two execution modes:

- **Interpreter mode:** evaluates the parsed abstract syntax tree (AST) directly in C without generating intermediate code. This fulfills the Phase 1 requirement.
- **Compiler mode:** emits LLVM IR from the same AST. Clang compiles that IR into a native executable ahead of time. This fulfills the Phase 2 requirement.

## Project Members

- Muhammad Faizan - 23L-0567
- Maahi Ibn Ahmar - 23L-0990
- Mahad Bin Atif - 22L-4951

## Supported Language Features

- Integer variables using `adad`
- Assignment and arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparisons: `chota hay` (`<`), `chota barabar` (`<=`), `bara hay` (`>`), `bara barabar` (`>=`), `barabar` (`==`), and `nahibarabar` (`!=`)
- Boolean operators: `&&`, `||`, `!`
- Conditional blocks using `agar` and `warna`
- While loops using `jabtak`
- Integer and string output using `dikhao`
- Program return values using `wapis`
- Statement termination using `kardo`
- Line comments beginning with `#` or `//`

## Arch Linux Dependencies

Arch Linux is the primary target platform. Install the required packages with:

```bash
sudo pacman -Syu --needed base-devel flex bison clang
```

Required tools:

- GCC or another C11 compiler for building the interpreter and compiler
- Flex 2.6 or newer for lexical analysis
- Bison 3.8 or newer for parsing
- Clang/LLVM for compiling generated LLVM IR
- GNU Make for the supplied build targets

No third-party C libraries are required.

## Compile the Project

From the project root, run:

```bash
make clean
make
```

This creates:

- `bin/roman-urdu` - unified interpreter and LLVM IR compiler

Both modes use the same scanner, grammar, AST, and executable. Select the required behavior with a command:

```bash
./bin/roman-urdu interpret program.ru
./bin/roman-urdu compile program.ru output.ll
./bin/roman-urdu build program.ru executable
```

## Interpreter Mode

Pass one Roman Urdu source file to the interpreter:

```bash
./bin/roman-urdu interpret examples/demo.ru
```

Expected output:

```text
y bara hai
1 se 5 tak jama:
15
```

The interpreter returns the value supplied to `wapis`. Syntax errors include their source line number. Semantic errors, such as use of a variable before declaration, are rejected before execution. Runtime errors, such as division by zero, are written to standard error.

## AOT Compiler Mode

Build a native executable directly:

```bash
./bin/roman-urdu build examples/demo.ru build/demo
./build/demo
```

The `build` command generates temporary LLVM IR, runs Clang, and removes the temporary file. To inspect or compile the LLVM IR manually, generate it with:

```bash
./bin/roman-urdu compile examples/demo.ru build/demo.ll
```

Compile the IR into a native executable:

```bash
clang build/demo.ll -o build/demo
```

Then run the executable:

```bash
./build/demo
```

Both workflows produce the same result as interpreter mode. If the IR output filename is omitted, compiler mode writes LLVM IR to standard output:

```bash
./bin/roman-urdu compile examples/demo.ru > build/demo.ll
```

The complete three-command workflow is also available through:

```bash
make demo
```

## Language Example

Roman Urdu source files conventionally use the `.ru` extension.

```text
adad n = 5 kardo
adad factorial = 1 kardo

jabtak (n bara hay 1) {
    factorial = factorial * n kardo
    n = n - 1 kardo
}

agar (factorial barabar 120) {
    dikhao("jawab durust hai") kardo
} warna {
    dikhao("jawab ghalat hai") kardo
}

dikhao(factorial) kardo
wapis 0 kardo
```

Keyword reference:

| Roman Urdu | Meaning |
| --- | --- |
| `adad` | Declare an integer variable |
| `agar` | If |
| `warna` | Else |
| `jabtak` | While |
| `dikhao` | Print a string or integer expression |
| `wapis` | Return from the program |
| `kardo` | End a statement |
| `barabar` | Equal comparison (`==`) |
| `nahibarabar` | Not-equal comparison (`!=`) |
| `bara hay` | Greater-than comparison (`>`) |
| `chota hay` | Less-than comparison (`<`) |
| `bara barabar` | Greater-than-or-equal comparison (`>=`) |
| `chota barabar` | Less-than-or-equal comparison (`<=`) |

Variables must be declared once, at top level, before they are assigned or used. The language uses a single program-wide integer namespace; declarations inside `agar`, `warna`, or `jabtak` blocks are rejected by the shared semantic checker. Integer expressions use signed 32-bit values. Interpreter and native modes both provide short-circuit Boolean evaluation and report division or modulo by zero.

## Run the Automated Tests

Build the unified program and run end-to-end tests for both modes:

```bash
make test
```

The test suite checks demo, factorial, comparison, and short-circuit programs in both modes. It also checks direct AOT builds, syntax errors, integer overflow, invalid declarations, use-before-declaration, and matching division/modulo-by-zero behavior. Sanitizer builds can be run by overriding `CFLAGS`.

Expected final line:

```text
All interpreter, compiler, semantic, and runtime tests passed.
```

## Optional Docker Verification

If Docker is available, reproduce the tested Arch Linux environment with:

```bash
docker build -t roman-urdu-compiler .
docker run --rm roman-urdu-compiler
```

The image builds the project and runs all tests during image creation. Its default command runs the demo.

## Project Files

- `src/lexer.l` - Shared Flex scanner for both execution modes.
- `src/parser.y` - Shared Bison grammar that builds the AST.
- `src/main.c` - Command-line entry point and mode selection.
- `include/ast.h` - AST node definitions and interpreter interface.
- `include/codegen.h` - LLVM IR code generator interface.
- `include/frontend.h` - Shared parser interface.
- `include/semantic.h` - Shared semantic validation interface.
- `src/ast.c` - AST construction, cleanup, symbol table, and interpreter execution.
- `src/semantic.c` - Declaration and variable-use validation shared by both modes.
- `src/codegen.c` - LLVM IR emission for expressions, variables, output, branches, and loops.
- `examples/demo.ru` - Full demonstration of variables, branches, loops, strings, and arithmetic.
- `examples/factorial.ru` - Small factorial example.
- `examples/comparisons.ru` - Equality and inequality keyword example.
- `tests/run_tests.sh` - End-to-end positive and negative regression tests.
- `tests/programs/` - Source programs used for semantic, parser, runtime, and short-circuit tests.
- `Makefile` - Linux build, test, demo, and cleanup targets.
- `Dockerfile` - Reproducible Arch Linux build and test environment.
- `README.md` - Markdown version of the project guide.
- `README.pdf` - Submission-ready PDF version of the project guide.
- `Roman_Urdu_Compiler.pdf` - Original project idea document.
- `LICENSE` - MIT license.

Generated files are placed under `build/` and `bin/`; both directories can be removed safely with `make clean`.

## Troubleshooting

- `flex: command not found` or `bison: command not found`: install the packages listed under Arch Linux Dependencies.
- `clang: command not found`: install it with `sudo pacman -S clang`.
- `line N: syntax error`: check for a missing `kardo`, parenthesis, or brace near the reported line.
- `semantic error`: declare each variable once at top level before assigning or reading it.
- `runtime error: division by zero` or `runtime error: modulo by zero`: correct the expression or input value.
- `Permission denied` when running a script: invoke it through `sh tests/run_tests.sh`; executable permission is not required in that form.

## Clean Generated Files

```bash
make clean
```

This removes only generated `bin/` and `build/` directories. Source files and documentation remain unchanged.
