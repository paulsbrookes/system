# Project Ideas

| Project | Key Syscalls | Description |
|---------|-------------|-------------|
| Mini shell | `fork`, `execvp`, `pipe`, `dup2`, `waitpid` | Pipes, redirects, background jobs |
| Custom malloc | `sbrk`, `mmap`, `munmap` | Heap allocator with free lists and coalescing |
| Fault handler | `mmap`, `mprotect`, `sigaction(SIGSEGV)` | Lazy allocation / copy-on-write via page faults |
| Shared memory ring buffer | `shm_open`, `mmap`, atomics | Lock-free IPC between processes |
| Build your own mutex | `futex(FUTEX_WAIT/WAKE)` | Implement mutex from scratch, benchmark vs pthread |
| Async event loop | `epoll`, `timerfd`, `signalfd`, `eventfd` | Single-threaded server (tiny libuv) |
| File change watcher | `inotify` | Watch directories, trigger actions on changes |
| Zero-copy file server | `sendfile`, `splice`, `socket` | HTTP file server with zero-copy I/O |
| Syscall sandbox | `seccomp`, `prctl` | BPF filter restricting syscalls for untrusted programs |
| Mini container | `clone(CLONE_NEW*)`, `pivot_root`, `mount` | Docker from scratch with namespaces |
| Mini strace | `ptrace(PTRACE_SYSCALL)` | Intercept and decode all syscalls |
