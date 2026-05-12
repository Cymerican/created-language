# Created Language — Cymerica Interpreter

A custom programming language interpreter built from scratch in C++ and Flex, developed across 4 progressive parts. Cymerica is a statically-typed, interpreted language featuring a hand-written recursive descent parser, a symbol table, and support for integer and real arithmetic.

---

## Project Progression

### Part 1 — Lexer & Basic Parser
- Built the Flex-based lexer (`rules.l`) to tokenize Cymerica source files
- Implemented a basic recursive descent parser
- Defined core token types in `lexer.h`
- Includes a test suite (`PartOneTests/`) with various input cases

### Part 2 — Symbol Table & Variable Support
- Added a symbol table using `std::map<string, std::variant<int, double>>`
- Introduced variable declarations and assignment statements
- Added the custom `CYMERICA` program entry keyword
- Supports `READ` statements for user input
- Includes sample `.tips` programs (the Cymerica file extension)

### Part 3 — REAL Datatype & Full Arithmetic
- Added `REAL` datatype with implicit type coercion between `INT` and `REAL`
- Full arithmetic with correct operator precedence
- Unary minus support
- `MOD` operator
- Compound assignment operators: `+=`, `-=`, `*=`, `/=`

### Part 4 — Extended Language Features
- Further language extensions and refinements
- Custom test program (`custom.tips`) demonstrating full language capabilities
- Expanded `rules.l` with additional token support

---

## Language Example

```
CYMERICA
  INT x
  REAL y
  READ x
  y = x * 2.5 + 1
  y += 10
  x MOD 3
```

---

## Project Structure

```
created-language/
├── part1/          # Lexer + basic parser
├── part2/          # Symbol table + variable support
├── part3/          # REAL datatype + full arithmetic
└── part4/          # Extended features
```

Each part contains:
- `rules.l` — Flex lexer rules
- `lexer.h` — Token definitions
- `parser.cpp` — Recursive descent parser and evaluator
- `ast.h` — Abstract syntax tree node definitions
- `driver.cpp` — Main entry point
- `debug.h` — Debug utilities
- `Makefile` — Build system

---

## How to Build and Run

### Prerequisites
- GCC / G++
- Flex
- Make

### Build (from any part directory)
```bash
cd part4
make
```

### Run
```bash
./parse your_program.tips
```

---

## How It Works

1. **Lexing** — `rules.l` is processed by Flex to generate a tokenizer that scans the source file and produces a stream of tokens.
2. **Parsing** — `parser.cpp` implements a recursive descent parser that consumes the token stream and evaluates expressions respecting operator precedence.
3. **Symbol Table** — Variables are stored in a `std::map<string, std::variant<int, double>>`, allowing the interpreter to handle both INT and REAL types dynamically.
4. **Evaluation** — Expressions are evaluated directly during parsing, with type coercion applied when INT and REAL values interact.

---

## Built With

- C++17
- Flex (Fast Lexical Analyzer)
- Make

---

## Author

**Cyrus** — Computer Science, Mississippi State University  
[GitHub](https://github.com/Cymerican)
