# Task Queue Implementation Examples

Three progressive task queue implementations in C, demonstrating the evolution from simple data structures to production-ready concurrent systems.

## Overview

1. **01_simple_queue.c** - Single-threaded FIFO queue using linked lists
2. **02_threadsafe_queue.c** - Multi-threaded queue with mutex synchronization
3. **03_threadpool.c** - Complete thread pool with worker threads

All examples process file operations (line counting, word counting, pattern matching) as concrete tasks.

## Building and Running

```bash
make                    # Build all examples
make clean              # Remove binaries

./01_simple_queue       # Run simple queue
./02_threadsafe_queue   # Run thread-safe queue
./03_threadpool         # Run thread pool

make run-all            # Run all in sequence
```

---

## Example 1: Simple Queue (01_simple_queue.c)

### Principles

Demonstrates fundamental **queue data structure** using a singly-linked list. A queue is a FIFO (First In, First Out) structure where items are added to the back and removed from the front.

**Key Implementation:**
- Maintains `head` pointer (front) and `tail` pointer (back) for O(1) enqueue/dequeue
- Each node allocated with `malloc()`, freed with `free()` on dequeue
- Simple sequential processing - one task after another

```c
typedef struct {
    QueueNode *head;  // Remove from here (dequeue)
    QueueNode *tail;  // Add here (enqueue)
    int count;
} TaskQueue;
```

### System Calls Used

**Memory Management:**
- `malloc()` - Allocate queue nodes dynamically
- `free()` - Deallocate nodes after processing

**File I/O:**
- `fopen()` - Open files for reading
- `fgetc()` - Read character-by-character (for line counting)
- `fgets()` - Read line-by-line (for pattern matching)
- `fclose()` - Close file handles

**String Operations:**
- `strncpy()` - Safe string copying for task fields
- `strstr()` - Substring search for pattern matching

---

## Example 2: Thread-Safe Queue (02_threadsafe_queue.c)

### Principles

Implements the **producer-consumer pattern** with thread-safe queue operations. Multiple threads can safely enqueue and dequeue tasks concurrently without race conditions.

**Key Implementation:**
- **Mutual exclusion** via `pthread_mutex_t` - only one thread modifies queue at a time
- **Condition variables** via `pthread_cond_t` - efficiently block consumers when queue is empty, wake them when tasks arrive
- **Critical sections** - all queue operations wrapped in mutex lock/unlock
- **Graceful shutdown** - special TASK_SHUTDOWN signals sent to each consumer

```c
pthread_mutex_lock(&queue->mutex);           // Enter critical section
queue_push(task);                            // Modify shared data
pthread_cond_signal(&queue->not_empty);      // Wake waiting consumer
pthread_mutex_unlock(&queue->mutex);         // Exit critical section
```

**Configuration:** 2 producer threads, 3 consumer threads

### System Calls Used

**Threading:**
- `pthread_create()` - Create producer and consumer threads
- `pthread_join()` - Wait for thread termination

**Synchronization:**
- `pthread_mutex_init()` - Initialize mutex lock
- `pthread_mutex_lock()` - Acquire exclusive access (blocks if already locked)
- `pthread_mutex_unlock()` - Release exclusive access
- `pthread_mutex_destroy()` - Clean up mutex

**Condition Variables:**
- `pthread_cond_init()` - Initialize condition variable
- `pthread_cond_wait()` - Atomically unlock mutex and block until signaled
- `pthread_cond_signal()` - Wake one waiting thread
- `pthread_cond_destroy()` - Clean up condition variable

**Timing:**
- `sleep()` - Delay between task creation (demonstration)
- `usleep()` - Microsecond delays to visualize thread interleaving

### Why Condition Variables?

Without condition variables, consumers would need to **busy-wait** (continuously check if queue has items), wasting CPU. Condition variables allow threads to sleep until work is available, then be woken efficiently by the OS scheduler.

---

## Example 3: Thread Pool (03_threadpool.c)

### Principles

Demonstrates the **thread pool pattern** used in production systems. Instead of creating/destroying threads for each task (expensive), a fixed pool of worker threads is created once and reused for all tasks.

**Key Implementation:**
- **Fixed-size worker pool** - 4 threads created at initialization, persist until shutdown
- **Work queue** - shared queue where main thread submits tasks, workers pull from
- **Active task tracking** - distinguishes between queued tasks and tasks currently being executed
- **Blocking wait** - `threadpool_wait()` blocks until all tasks (queued + active) complete
- **Clean API** - encapsulates complexity: `create()`, `submit()`, `wait()`, `destroy()`

```c
typedef struct {
    pthread_t *threads;           // Worker thread array
    QueueNode *queue_head;        // Task queue
    pthread_mutex_t mutex;
    pthread_cond_t task_available; // Wake workers when task added
    pthread_cond_t queue_empty;    // Signal when all work done
    int shutdown;                  // Shutdown flag
    int active_tasks;              // Currently executing tasks
} ThreadPool;
```

**Worker Thread Lifecycle:**
1. Created at pool initialization
2. Loop: wait for task → execute task → repeat
3. On shutdown signal with empty queue: exit loop
4. Join in `threadpool_destroy()`

### System Calls Used

Same threading and synchronization calls as Example 2, plus:

**Advanced Synchronization:**
- `pthread_cond_broadcast()` - Wake **all** waiting threads (used for shutdown)
- `pthread_cond_wait()` on `queue_empty` - Main thread waits for workers to finish all tasks

**Why Two Condition Variables?**

- `task_available`: Workers wait on this when queue is empty, signaled when tasks submitted
- `queue_empty`: Main thread waits on this in `threadpool_wait()`, signaled when queue empty AND no active tasks

This allows the main thread to block until all work completes, not just until queue drains.

---

## System Calls Summary

### All Examples
| Call | Purpose |
|------|---------|
| `malloc()`, `free()` | Dynamic memory allocation for queue nodes |
| `fopen()`, `fclose()` | File handle management |
| `fgetc()`, `fgets()` | File reading for task execution |
| `strncpy()`, `strstr()` | String operations |
| `printf()`, `perror()` | Output and error reporting |

### Threading Examples (02 & 03)
| Call | Purpose |
|------|---------|
| `pthread_create()` | Spawn new threads |
| `pthread_join()` | Wait for thread termination |
| `pthread_mutex_init/lock/unlock/destroy()` | Mutual exclusion (prevent race conditions) |
| `pthread_cond_init/wait/signal/broadcast/destroy()` | Condition variables (efficient blocking/waking) |
| `sleep()`, `usleep()` | Timing delays (demonstration purposes) |

---

## Core Principles Demonstrated

### Data Structures
- **Linked list** queue with O(1) enqueue/dequeue via head/tail pointers
- Dynamic memory management with proper allocation/deallocation

### Concurrency
- **Critical sections** - protecting shared data with mutexes
- **Condition variables** - blocking threads efficiently until work is available
- **Producer-consumer pattern** - decoupling task creation from execution
- **Thread pool pattern** - amortizing thread creation cost across many tasks

### Synchronization Patterns
- **Wait loop pattern**: `while (condition) { pthread_cond_wait(...); }`
  (protects against spurious wakeups)
- **Signal after unlock**: Release mutex before signaling to avoid waking thread that immediately blocks again
- **Active task tracking**: Count both queued and executing tasks to know when all work is done
- **Broadcast for shutdown**: Wake all threads simultaneously to terminate

---

## Compilation Details

```bash
gcc -Wall -Wextra -std=c99 -O2 01_simple_queue.c -o 01_simple_queue
gcc -Wall -Wextra -std=gnu99 -O2 -pthread 02_threadsafe_queue.c -o 02_threadsafe_queue
gcc -Wall -Wextra -std=gnu99 -O2 -pthread 03_threadpool.c -o 03_threadpool
```

- `-pthread`: Link pthread library and enable thread-safe compilation
- `-std=gnu99`: GNU C99 for POSIX extensions like `usleep()`

---

## Performance Notes

- **Simple Queue**: O(1) enqueue/dequeue, no synchronization overhead
- **Thread-Safe Queue**: Mutex lock/unlock on every operation, context switching overhead
- **Thread Pool**: Amortizes thread creation cost, fixed pool size prevents resource exhaustion

---

## Real-World Usage

This thread pool pattern appears in:
- **Web servers** (nginx, Apache) - handle requests with worker threads
- **Databases** (PostgreSQL) - background maintenance workers
- **Message queues** (Redis, RabbitMQ) - task distribution
- **Build systems** (make -j) - parallel job execution
