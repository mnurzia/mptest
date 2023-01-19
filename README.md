# mptest

Small, modular unit-testing library written in C89.

Features:

- Fault checking (mock support)
  - Simulates OOM (out-of-memory) errors out-of-the-box by using a custom `malloc()`
  - Versatile enough to adapt for simulating other types of faults (I/O errors, thread initialization errors, etc.)
- Memory leak checking support
  - Tracks heap usage at exit and displays remaining allocations
  - Remembers allocation history (tracks memory across calls to `realloc()`)
- Custom data-type S-expression support:
  - Allows easy creation of test example data through typed s-expressions
  - Example (taken from `re`):
    ```lisp
    (ast
    (concat (
      (rune 'q')
      (alt (
        (rune 'a')
        (rune 'b')
        (rune 'c')
        (rune 'd'))))))
    ```
- Fuzzing support
  - Run tests with (optionally deterministic) random parameters
  - Run tests multiple times with different parameters
- Only ~6300 lines of code as of Jan 2023
