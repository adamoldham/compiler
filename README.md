# lille Compiler

Compiler in C++20 for the programming language lille, built in phases on top of provided starter kit.

## Repository Layout

```
compiler-project/
  compiler/   Compiler source (starter kit + implementation)
  pal/        PAL machine interpreter and sample PAL programs
  tests/      additional lille test programs, grouped by phase
  README.md
  .gitignore
```

- `compiler/`: scanner, token and symbol classes, error handler, identifier table, driver (`compiler.cpp`).
- `pal/`: the PAL interpreter used as the target machine. Not modified for the required phases.
- `tests/`: lille programs written for testing, split into `scanner/`, `parser/`, `semantics/`, and `recovery/`.

## Build

Requires a C++20 compiler (g++ or clang++) and make.

```
cd compiler
make

cd ../pal
make
```

## Run

```
cd compiler
./compiler [flags] <source-file>
```

Flags:

- `-h`: print a help message.
- `-l`: create a listing file showing the source and any errors (not implemented yet).
- `-o filename`: name of the generated PAL code file.

The driver creates the scanner and the recursive-descent parser and parses the whole program. A valid program finishes with 0 errors. A program with a syntax error reports the first error (message, error number, line and position) and then stops, so the final count is 1. Set `debugging` to `true` in `scanner.h` and rebuild to print each line read and each token returned.

To run PAL object code:

```
cd pal
./pal <program>.pal
```

## Testing

Test programs live in `tests/`, one folder per phase. Each construct gets at least one valid and one malformed example. Earlier phase tests are rerun after each new phase to catch regressions.

Run the sample programs that came with the starter kit:

```
cd compiler
for f in program1 program2 program3 program4 program5 program6 program7 program8 program9; do echo "== $f"; ./compiler $f; done
```

Each of `program1` to `program9` should finish with 0 errors. `program1_test` is `program1` without the `program ... is` header, so it should report 1 error (#47, PROGRAM symbol expected) at line 1.

Run the parser tests:

```
cd compiler
for f in ../tests/parser/*.lille; do echo "== $f"; ./compiler $f; done
```

Files named `valid_*.lille` finish with 0 errors. Files named `err_*.lille` each report exactly 1 error, because the parser stops at the first syntax error. `valid_edge_cases.lille` also lists, in comments, constructs the grammar rejects on purpose.

Run the scanner tests:

```
cd compiler
for f in ../tests/scanner/*.lille; do echo "== $f"; ./compiler $f; done
```

The scanner test files are lists of tokens, not complete programs. Since the parser was added, it stops at the first token of each one with error #47 (PROGRAM symbol expected), so the scanner no longer reads past the first token through the driver. `scanner.cpp` is unchanged from phase 1.

## Known Issues

- Scanner `error_message()` is off by one for when/while/write/writeln (the message table has no WHEN entry), so `must_be()` on those tokens would print the wrong text. The parser only calls it on them after `have()` has matched.
- Scanner: exponents with more than 3 digits are rejected (error 65).
- Scanner: `12E3` (no sign after E) is accepted.
- Scanner: an empty string `""` is accepted.
- Scanner: a lone period reports error 22 instead of 74.

## Parser Notes

- LL(1) recursive descent in `compiler/parser.cpp` and `compiler/parser.h`, one function per grammar nonterminal.
- Only the first syntax error is reported. `expect()` wraps `must_be()` and throws once an error is reported.
- Decisions: `eof` is not accepted in expressions (it is not in the grammar). A `(` directly after `write` or `writeln` is always the wrapper parenthesis. `not` and `odd` take the longest expression, so `not a = b` means `not (a = b)`.
- Syntax only. Declarations, types and scope are checked in phase 3.

## Notes

- Language reference: the lille syntax and semantics handout is the authority for grammar and semantic rules.
- README created by Claude and manually checked for errors