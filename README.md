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

At this stage the driver requests tokens from the scanner until the end of the program. Set `debugging` to `true` in `scanner.h` and rebuild to print each line read and each token returned.

To run PAL object code:

```
cd pal
./pal <program>.pal
```

## Testing

Test programs live in `tests/`, one folder per phase. Each construct gets at least one valid and one malformed example. Earlier phase tests are rerun after each new phase to catch regressions.

Run the sample programs that came with the starter kit (each should finish with 0 errors):

```
cd compiler
for f in program1 program1_test program2 program3 program4 program5 program6 program7 program8 program9; do echo "== $f"; ./compiler $f; done
```

Run the scanner tests:

```
cd compiler
for f in ../tests/scanner/*.lille; do echo "== $f"; ./compiler $f; done
```

Expected error counts: `err_numbers.lille` 5, `all_nums.lille` 5, `err_strings.lille` 4. All other scanner tests finish with 0 errors.

## Notes

- Language reference: the lille syntax and semantics handout is the authority for grammar and semantic rules.
- Repo on GitLab not GitHub (unless I need to switch)