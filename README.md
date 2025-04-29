# Usage

## Compiler

Run the compiler from the command line. You can provide the source file and output file as arguments, or use standard input/output.

### Using stdin/stdout:

```bash
./ifj24_compiler < path/to/your_source.ifj24 > path/to/output.ifjcode24
```

### Using file arguments:

```bash
./ifj24_compiler path/to/your_source.ifj24 path/to/output.ifjcode24
```

### Compiler Exit Codes:

- `0`: Success
- `1`: Lexical error
- `2`: Syntax error
- `3`: Semantic error (undefined function/variable)
- `4`: Semantic error (function call parameter mismatch)
- `5`: Semantic error (redefinition, assignment to const)
- `6`: Semantic error (return statement issue)
- `7`: Semantic error (type compatibility)
- `8`: Semantic error (type inference failure)
- `9`: Semantic error (unused variable)
- `10`: Other semantic errors
- `99`: Internal compiler error (e.g., memory allocation failure)

---

## Interpreter

An interpreter (`ic24int`) is provided to execute the generated IFJcode24 code.

### Running the interpreter:

```bash
./ic24int path/to/output.ifjcode24 < path/to/program_input.in > path/to/program_output.out
```

Use `./ic24int --help` for more options.

### Interpreter Exit Codes:

- `0`: Success
- `50`: Bad parameters for interpreter
- `51`: Error reading input code (lexical/syntax)
- `52`: Semantic error in input code
- `53`: Runtime error (bad operand types)
- `54`: Runtime error (accessing non-existent variable)
- `55`: Runtime error (frame does not exist)
- `56`: Runtime error (missing value)
- `57`: Runtime error (bad operand value - division by zero, bad EXIT value)
- `58`: Runtime error (string operation error)
- `60`: Internal interpreter error

---

## Testing

A Python script (`all_in_one.py`) is included for running tests.

### Running tests:

```bash
python3 all_in_one.py --tests-dir path/to/test/files/directory
```

The script performs the following:

- Compiles `.ifj24` files from the specified directory using `./ifj24_compiler`.
- Runs the generated `.ifjcode24` files using `./ic24int`.
- Checks exit codes and compares the program output (if applicable).
- Reads input for the interpreted program from corresponding `.input` files, or uses `'4'` as a default input if not provided.
- Use the `--verbose` flag for detailed interpreter output.

---

## Language Notes

### IFJ24 (Source Language)

- A simplified subset of Zig.
- Strongly typed.
- Programs require a prolog:

```zig
const ifj = @import("ifj24.zig");
```

- Supports:
  - Functions
  - Variables (`var`, `const`)
  - Control structures (`if/else`, `while`)
  - `return` statements

### IFJcode24 (Target Language)

- Intermediate code with three-address and stack-based instructions.
- Case-insensitive opcodes, case-sensitive operands.
- Memory model uses three frames:
  - Global Frame (GF)
  - Local Frame (LF)
  - Temporary Frame (TF)
- Code must start with `.IFJcode24` header.
- Supports single-line comments starting with `#`.
