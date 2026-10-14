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
./compiler <source-file>.lille
```

At this stage the driver requests tokens from the scanner and prints them. Confirm the exact command-line usage against `compiler.cpp` and update this section if it differs.

To run PAL object code:

```
cd pal
./pal <program>.pal
```

## Testing

Test programs live in `tests/`, one folder per phase. Each construct gets at least one valid and one malformed example. Earlier phase tests are rerun after each new phase to catch regressions.

## Notes

- Language reference: the lille syntax and semantics handout is the authority for grammar and semantic rules.
- Repo on GitLab not GitHub (unless I need to switch)