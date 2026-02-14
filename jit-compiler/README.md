# Brainfuck JIT Compiler

A just-in-time compiler that translates Brainfuck source to x86-64 machine code at runtime and executes it directly.

## Key Concepts

- **W^X code generation** — `mmap(PROT_READ|PROT_WRITE)` to emit code, then `mprotect(PROT_READ|PROT_EXEC)` before execution. Memory is never writable and executable simultaneously.
- **x86-64 instruction encoding** — REX prefixes, ModR/M bytes, SIB bytes, and immediate operands assembled by hand.
- **Calling convention interop** — JIT'd code calls libc `putchar`/`getchar` via `call rax` with correct System V ABI register setup.
- **Backpatching** — `[` emits a forward `je` with a placeholder displacement; `]` patches it once the target address is known.
- **Run-length optimisation** — consecutive `+`/`-`/`>`/`<` are collapsed into single instructions.

## Usage

```
./jit [options] <file.bf>
./jit [options] -e '<brainfuck code>'

Options:
  -v    Verbose mode: show generated machine code disassembly
  -e    Execute inline code instead of reading a file
```

## Build

```
make        # build
make test   # run verification tests
make clean  # remove binary
```

## Register Allocation

| Register | Purpose |
|----------|---------|
| `r12` | Tape base pointer (callee-saved, survives calls) |
| `r13` | Current cell offset (data pointer) |
| `rdi` | Argument to `putchar`/`getchar` |
| `rax` | Return value / scratch / function pointer for indirect calls |

## x86-64 Encoding Reference

| BF | x86-64 | Hex Encoding | Bytes |
|----|--------|-------------|-------|
| `+` (x1) | `inc byte [r12+r13]` | `43 fe 04 2c` | 4 |
| `+` (xN) | `add byte [r12+r13], N` | `43 80 04 2c NN` | 5 |
| `-` (x1) | `dec byte [r12+r13]` | `43 fe 0c 2c` | 4 |
| `-` (xN) | `sub byte [r12+r13], N` | `43 80 2c 2c NN` | 5 |
| `>` (x1) | `inc r13` | `49 ff c5` | 3 |
| `>` (xN, imm8) | `add r13, N` | `49 83 c5 NN` | 4 |
| `>` (xN, imm32) | `add r13, N` | `49 81 c5 NN..` | 7 |
| `<` (x1) | `dec r13` | `49 ff cd` | 3 |
| `<` (xN, imm8) | `sub r13, N` | `49 83 ed NN` | 4 |
| `<` (xN, imm32) | `sub r13, N` | `49 81 ed NN..` | 7 |
| `.` | `movzx edi,[r12+r13]` / `mov rax,putchar` / `call rax` | 5+10+2 | 17 |
| `,` | `mov rax,getchar` / `call rax` / `mov [r12+r13],al` | 10+2+4 | 16 |
| `[` | `cmp byte [r12+r13],0` / `je rel32` | 5+6 | 11 |
| `]` | `cmp byte [r12+r13],0` / `jne rel32` | 5+6 | 11 |

## W^X (Write XOR Execute)

Modern security practice dictates that memory should never be both writable and executable simultaneously. This JIT follows that principle:

1. `mmap(PROT_READ | PROT_WRITE)` — allocate a 1 MiB code buffer, writable but not executable
2. Emit all machine code into the buffer
3. `mprotect(PROT_READ | PROT_EXEC)` — make executable, remove write permission
4. Cast buffer to function pointer and call it
5. `munmap()` — release when done

## Verification Programs

**Hello World:**
```
./jit -e '++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.'
```

**Addition (2+5=7):**
```
./jit -e '++>+++++<[->+<]>++++++++++++++++++++++++++++++++++++++++++++++++.'
```

**Cat (echo stdin):**
```
./jit -e ',[.,]'
```
