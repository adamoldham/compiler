# lille Compiler

Individual project for CSC4510 Programming Language Design and Translation at High Point University. A compiler in C++20 for the programming language lille, built in phases on top of an instructor-provided starter kit.

## Repository Layout

```
compiler-project/
  compiler/   Compiler source (starter kit plus my implementation)
  pal/        PAL machine interpreter and sample PAL programs
  tests/      My own lille test programs, grouped by phase
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

## Phase Status

| Phase | Due | Status | Git tag |
|---|---|---|---|
| Scanner | Oct 22 | In progress | `scanner-submission` |
| Parser | Nov 8 | Not started | `parser-submission` |
| Static Semantic Analysis | Nov 22 | Not started | `semantics-submission` |
| Error Recovery and Completed Compiler | TBA | Not started | `final-submission` |
| Code Generation (optional) | TBA | Not started | |

## Branching Workflow

- `main` holds the last stable, submitted snapshot. The first commit is the untouched starter kit.
- Each phase is developed on its own branch (`phase1-scanner`, `phase2-parser`, and so on), then merged into `main` with `--no-ff` and tagged.
- Phase tags are pushed to every remote so each submission snapshot is preserved.

## Testing

Test programs live in `tests/`, one folder per phase. Each construct gets at least one valid and one malformed example. Earlier phase tests are rerun after each new phase to catch regressions.

## Notes

- Language reference: the lille syntax and semantics handout is the authority for grammar and semantic rules.
- Repository visibility: kept private per the instructor's sharing instructions for graded individual work.
