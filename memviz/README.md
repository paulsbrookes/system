# memviz - Process Memory Visualizer

An interactive terminal-based tool for visualizing memory allocation in running processes. Uses the Linux `/proc` filesystem to gather detailed memory information and presents it through a multi-level ncurses interface.

## Purpose

This program helps you understand how memory is allocated in a running process by:
- Showing overall memory usage (virtual and physical)
- Breaking down memory into segments (heap, stack, code, libraries)
- Displaying detailed memory maps with addresses and permissions
- Calculating percentages relative to system memory

## Features

- **Three View Levels** - Navigate between different detail levels:
  1. **Overview** - High-level memory statistics with visual bar graphs
  2. **Segments** - Memory categorized by type (heap, stack, libraries, etc.)
  3. **Detailed Maps** - Full memory map with addresses, permissions, and paths

- **Interactive Navigation** - Use number keys (1, 2, 3) to switch views
- **Scrolling** - Arrow keys and Page Up/Down for detailed maps view
- **Visual Bar Graphs** - ASCII bar charts showing memory percentages
- **Color Support** - Color-coded output for better readability

## System Calls Used

### File I/O System Calls
- **`fopen()` / `fgets()` / `fclose()`** - Read data from `/proc` filesystem files

### /proc Filesystem Files
While not technically system calls, these special files provide kernel data:
- **`/proc/[pid]/status`** - Process memory statistics (VmSize, VmRSS, VmData, etc.)
- **`/proc/[pid]/maps`** - Detailed memory mappings with addresses and permissions
- **`/proc/meminfo`** - System-wide memory information (MemTotal)

## Concepts Demonstrated

### Memory Management Concepts

1. **Virtual Memory (VmSize)** - Total address space allocated to the process
   - May be much larger than physical RAM
   - Includes memory that hasn't been accessed yet (demand paging)

2. **Resident Set Size (RSS)** - Physical memory actually in RAM
   - The "real" memory usage
   - What you see in `top` or `htop`

3. **Memory Segments**:
   - **Heap** - Dynamically allocated memory (`malloc()`, `new`)
   - **Stack** - Function call frames and local variables
   - **Code** - Executable instructions
   - **Data** - Global and static variables
   - **Libraries** - Shared libraries (.so files)

4. **Memory Mappings**:
   - Each region has start/end addresses
   - Permissions: r (read), w (write), x (execute), p (private)
   - Can be file-backed or anonymous

### Programming Concepts

- **String Parsing** - Extracting data from /proc text files
- **Data Structures** - Arrays and structs for organizing memory data
- **ncurses** - Full-screen terminal UI with windows and colors
- **State Management** - View levels and scrolling state
- **Categorization Logic** - Grouping memory maps by type

## Usage

```bash
./memviz <pid>
```

**Example:**
```bash
# Find a process ID
pidof firefox
# or
ps aux | grep firefox

# Visualize its memory
./memviz 12345
```

### Navigation

- **`1`** - Switch to Overview view
- **`2`** - Switch to Segments view
- **`3`** - Switch to Detailed Maps view
- **`↑` / `↓`** - Scroll up/down (in Detailed Maps view)
- **`PgUp` / `PgDn`** - Scroll by page (in Detailed Maps view)
- **`q`** - Quit

## View Descriptions

### 1. Overview
Shows high-level statistics from `/proc/[pid]/status`:
- Total virtual memory and RSS
- Percentage of system RAM used
- Breakdown by data, stack, code, and libraries
- Visual bar graphs for quick comparison

### 2. Segments
Categorizes all memory mappings into logical groups:
- Heap memory (dynamic allocations)
- Stack memory (call stack)
- Code/executables
- Shared libraries
- Anonymous mappings
- Other mappings

Parses `/proc/[pid]/maps` and groups regions by pathname and type.

### 3. Detailed Maps
Shows every individual memory mapping:
- Start and end addresses (hexadecimal)
- Size in KB or MB
- Permissions (r/w/x/p)
- Associated file or special region name

Useful for understanding the complete memory layout.

## Compilation

```bash
gcc -o memviz memviz.c -lncurses
```

Or use the provided Makefile:
```bash
make
```

## Example Output

```
MEMORY OVERVIEW - Process 12345

System Total Memory: 16384 MB

Virtual Memory (VmSize):
  Total Virtual         [====================--------------------]   512000 KB ( 50.0%)

Physical Memory (RSS):
  Resident Set Size     [==========------------------------------]   256000 KB ( 25.0%)

Memory Breakdown:
  Data Segment          [========--------------------------------]   204800 KB ( 40.0%)
  Stack                 [=---------------------------------------]    40960 KB (  8.0%)
  Code (Executable)     [==--------------------------------------]    51200 KB ( 10.0%)
  Libraries             [==========------------------------------]   256000 KB ( 50.0%)
```

## Understanding the Data

### What's the difference between VmSize and VmRSS?

- **VmSize** - Virtual memory is the total address space reserved
- **VmRSS** - Resident memory is what's actually loaded in physical RAM

A process might reserve 1 GB (VmSize) but only use 100 MB (VmRSS).

### Why are there so many memory mappings?

Each shared library, each mmap'd file, and different memory regions (heap, stack, code) appear as separate mappings. A typical program has 100+ mappings.

### What are anonymous mappings?

Memory not backed by a file. Created by `malloc()`, `mmap(MAP_ANONYMOUS)`, or used for the heap and stack.

## Understanding the Three Views

### View 1: Overview - The Big Picture

**What it shows:**
- **VmSize** - Total virtual address space reserved by the process
- **VmRSS** - Physical RAM actually being used right now
- **VmData** - Data segment (heap, globals, mmap'd regions)
- **VmStk** - Stack size
- **VmExe** - Executable code size
- **VmLib** - Shared libraries size

**Key insight:** Shows the difference between what's *reserved* (VmSize) and what's *actually using RAM* (VmRSS). Modern applications often reserve far more virtual memory than they use physically.

**Example:** A Chrome process might show:
- VmSize: 1.2 TB (reserved virtual space)
- VmRSS: 450 MB (actual RAM usage)

Only the RSS matters for your system's memory pressure.

### View 2: Segments - What's the Memory For?

**What it shows:**

Categorizes memory by purpose, parsed from `/proc/[pid]/maps`:

- **Heap** - Regions marked `[heap]` or custom allocators
- **Stack** - Thread stacks marked `[stack]`
- **Code** - Executable files (your program binary)
- **Libraries** - Shared objects (`.so` files)
- **Anonymous** - Unnamed memory regions (malloc, mmap)
- **Other** - Special regions (vsyscall, vdso, vvar, etc.)

**Key insight:** Modern applications may not use traditional `[heap]`. Chrome uses PartitionAlloc, which appears as anonymous memory. Similarly, no `[heap]` doesn't mean no dynamic allocation!

**What you learn:**
- How much goes to your code vs. dependencies
- Stack size (often surprisingly small, <1 MB)
- Heap allocation patterns

### View 3: Detailed Maps - Every Memory Region

**What it shows:**

Every individual mapping from `/proc/[pid]/maps`:

```
START_ADDR - END_ADDR   SIZE   PERMS  PATHNAME
```

**Columns explained:**
- **START/END** - Virtual address range (hexadecimal)
- **SIZE** - Region size in KB or MB
- **PERMS** - Four characters: r (read), w (write), x (execute), p (private/shared)
- **PATHNAME** - File backing this region, or special tag like `[stack]`, `[anon:v8]`

**Key patterns to notice:**

1. **Guard pages** - Regions with `---p` (no permissions)
   - Catch buffer overflows by causing segfaults
   - Common in security-conscious allocators

2. **Code/data separation** - Same file mapped multiple times:
   ```
   /bin/program  r-xp   ← executable code (can't write)
   /bin/program  r--p   ← read-only data
   /bin/program  rw-p   ← writable data
   ```

3. **Fragmentation** - Hundreds of small regions
   - Each library, each allocation pool creates entries
   - Modern apps easily have 1000+ mappings

**What you learn:**
- Exact memory layout of the process
- Security boundaries (which regions can execute, which can write)
- File-backed vs anonymous memory
- Allocator implementation details (guard pages, alignment)

## Common Misconceptions

### ❌ "High VmSize means the app is using lots of RAM"

**Reality:** VmSize is *virtual address space*. On 64-bit systems, this is essentially free. An app can reserve terabytes of virtual space while using only megabytes of physical RAM.

**Why it's misleading:** Modern allocators (like Chrome's PartitionAlloc) reserve huge contiguous regions for security and performance but only commit pages as needed.

**What to check instead:** Look at **VmRSS** for actual RAM usage.

### ❌ "VmData should be smaller than VmRSS"

**Reality:** VmData can be *much larger* than VmRSS due to demand paging.

**Why this happens:**
- Process allocates 1 GB with `malloc()` → VmData increases
- Process only writes to 100 MB → VmRSS increases by 100 MB
- The remaining 900 MB is "allocated" but never backed by physical pages

**This is normal and efficient** - you only pay (in RAM) for what you touch.

### ❌ "More memory mappings = worse performance"

**Reality:** Number of mappings has minimal performance impact.

**Why it's misleading:** Having 1000 tiny mappings vs. 10 large ones doesn't significantly affect runtime. The kernel handles this efficiently with page tables.

**What matters more:** Total RSS, memory access patterns, cache locality.

### ❌ "Guard pages (`---p`) are wasted space"

**Reality:** Guard pages cost almost nothing and provide crucial security.

**Why they're valuable:**
- Catch buffer overflows before they corrupt data
- Detect use-after-free bugs
- Only reserve virtual addresses, no physical RAM
- Cause immediate crashes instead of silent corruption

**Modern best practice:** Security-conscious allocators use guard pages liberally.

### ❌ "Stack is tiny, so it's not important"

**Reality:** Stack is small (often <1 MB) *by design*.

**Why it's small:**
- Most data goes on the heap (via `malloc`/`new`)
- Deep recursion quickly overflows stack
- Threads each get their own stack, so keeping it small saves space

**What this means:** If you see stack overflow errors, it's not because the stack is "too small" - it's because your code has excessive recursion or huge local arrays.

### ❌ "No `[heap]` region means no dynamic allocation"

**Reality:** Modern allocators don't always use traditional `[heap]`.

**Examples:**
- **Chrome** - Uses PartitionAlloc (appears as `[anon:partition_alloc]`)
- **jemalloc** - Creates anonymous mappings
- **tcmalloc** - Uses its own regions

**What to look for:** Anonymous (`rw-p`) regions without filenames - that's your heap!

### ❌ "Shared libraries save memory automatically"

**Reality:** Libraries only save memory if *actually shared* between processes.

**Nuances:**
- Private mappings (`rw-p`) are not shared, even if from a `.so` file
- Modern security features (ASLR, PIE) can prevent sharing
- RSS includes your private portions of "shared" libraries

**To verify:** Use `pmap -x <pid>` and check the "Shared" column, or tools like `smem`.

## Permissions to Monitor Processes

You can monitor:
- Your own processes (always)
- Other users' processes (only if you're root)

If you get "Cannot read memory stats", the process either doesn't exist or you lack permissions.

## Related System Tools

- **`top`** / **`htop`** - Real-time process memory monitoring
- **`pmap <pid>`** - Display memory maps (similar to detailed view)
- **`cat /proc/<pid>/maps`** - Raw memory map data
- **`smem`** - Memory reporting with shared memory awareness

## See Also

- `/proc` filesystem documentation: `man 5 proc`
- Memory management concepts: `man 7 memory`
- ncurses programming: `man 3 ncurses`
- Virtual memory: `man 2 mmap`

## Technical Notes

- Single snapshot: reads data once at startup (no live updates)
- Limited to 1000 memory mappings (most processes have far fewer)
- Assumes 4 KB page size
- Color support detected automatically
- Designed for 80+ column terminals
