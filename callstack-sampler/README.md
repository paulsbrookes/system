# callstack-sampler - Sampling Profiler via ptrace

A sampling profiler that periodically captures the callstack of a running C program using `ptrace`. Demonstrates ptrace-based process introspection, signal handling, and frame pointer chain walking — the same conceptual approach used by real profilers like `perf`.

Target programs must be compiled with `-fno-omit-frame-pointer` so the RBP chain can be walked.

## Usage

```bash
./sampler <pid> [interval_ms]
```

- `pid` — target process ID (required)
- `interval_ms` — sampling interval in milliseconds (default: 100)

**Example:**

```bash
# Compile a test program with frame pointers preserved
cat > /tmp/test_target.c << 'EOF'
void c(void) { while(1); }
void b(void) { c(); }
void a(void) { b(); }
int main(void) { a(); return 0; }
EOF
gcc -fno-omit-frame-pointer -O0 -o /tmp/test_target /tmp/test_target.c

# Run target in background
/tmp/test_target &
TARGET_PID=$!

# Sample its callstack
./sampler $TARGET_PID

# Press Ctrl+C to stop — target continues running normally
kill $TARGET_PID
```

## Output Format

```
Attached to process 12345, sampling every 100 ms
Press Ctrl+C to stop

--- sample 1 ---
  0x5555555551a0
  0x555555555220
  0x555555555240
  0x7ffff7e1a000
--- sample 2 ---
  0x5555555551c8
  0x555555555220
  0x555555555240
  0x7ffff7e1a000
```

Each sample shows the instruction pointer (RIP) at the top, followed by return addresses from the frame pointer walk. The deepest caller appears at the bottom.

## How It Works

1. **Attach** to the target process with `ptrace(PTRACE_ATTACH)`
2. **Read registers** to get RIP (current instruction) and RBP (frame pointer)
3. **Walk the frame pointer chain**: at each frame, read the return address at `RBP+8` and the saved RBP at `RBP`, following the chain until it terminates
4. **Resume** the target with `ptrace(PTRACE_CONT)`
5. **Sleep** for the sampling interval
6. **Stop** the target again with `kill(pid, SIGSTOP)` and repeat
7. On Ctrl+C, **detach** cleanly so the target continues running

## System Calls Used

- **`ptrace(PTRACE_ATTACH)`** — attach to target process (sends SIGSTOP)
- **`ptrace(PTRACE_DETACH)`** — detach, allowing target to resume normally
- **`ptrace(PTRACE_CONT)`** — resume a stopped target process
- **`ptrace(PTRACE_GETREGS)`** — read target's CPU registers (RIP, RBP, etc.)
- **`ptrace(PTRACE_PEEKDATA)`** — read a word from the target's memory
- **`waitpid()`** — synchronize with target stop/continue state changes
- **`kill(pid, SIGSTOP)`** — stop target process for sampling
- **`sigaction()`** — install SIGINT handler for clean shutdown
- **`usleep()`** — sleep between samples

## Concepts Demonstrated

- **Sampling profilers** — periodically interrupting a program to observe where it spends time, rather than instrumenting every function call
- **ptrace** — the Linux system call for process tracing and debugging (used by gdb, strace, etc.)
- **Frame pointer chain walking** — following the RBP register chain to reconstruct the callstack, which requires `-fno-omit-frame-pointer`
- **Signal handling** — using `sigaction` and `volatile sig_atomic_t` for safe async signal handling
- **Process control** — stopping and resuming another process from userspace

## x86-64 Stack Frame Layout

When compiled with `-fno-omit-frame-pointer`, each function's stack frame looks like:

```
Higher addresses (stack grows down)
┌──────────────────┐
│   arguments       │
├──────────────────┤
│   return address  │  ← RBP + 8
├──────────────────┤
│   saved RBP       │  ← RBP points here
├──────────────────┤
│   local variables │
├──────────────────┤
│   ...             │  ← RSP points to top
└──────────────────┘
Lower addresses
```

The sampler reads `RBP+8` to get the return address, then reads `RBP` to get the caller's frame pointer, and repeats.

## Limitations

- **x86-64 only** — relies on RBP/RIP register conventions
- **Requires frame pointers** — target must be compiled with `-fno-omit-frame-pointer`
- **Addresses only** — does not resolve addresses to function names (use `addr2line` or `nm` for that)
- **Same user or root** — ptrace requires appropriate permissions
- **No ASLR compensation** — addresses change between runs

## Resolving Addresses to Function Names

Use `addr2line` to map addresses back to source locations:

```bash
addr2line -e /path/to/binary -f 0x5555555551a0
```

Or use `nm` to list all symbol addresses:

```bash
nm /path/to/binary | sort
```

## Compilation

```bash
make
```

Or directly:

```bash
gcc -Wall -Wextra -std=c99 -o sampler sampler.c
```

## See Also

- `man 2 ptrace` — process trace system call
- `man 2 waitpid` — wait for process state changes
- `man 7 signal` — signal handling overview
- `perf` — Linux performance analysis tool (uses similar sampling approach)
