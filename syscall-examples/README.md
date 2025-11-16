# System Call Examples

This directory contains examples demonstrating various Linux system calls.

## Examples

### 1. File I/O System Calls

**File**: `file_io_syscalls.c`

Demonstrates:
- `open()` - Opening/creating files
- `write()` - Writing data to files
- `read()` - Reading data from files
- `close()` - Closing file descriptors

**Compile and run**:
```bash
make file_io_syscalls
./file_io_syscalls
```

Or:
```bash
make run-file-io
```

### 2. Named Pipes (FIFOs)

**Files**: `named_pipe_writer.c`, `named_pipe_reader.c`

#### What are Named Pipes?

Named pipes (also called FIFOs - First In First Out) are a form of inter-process communication (IPC) in Unix-like systems. Unlike anonymous pipes (created with `pipe()`), named pipes:

- Exist in the filesystem as special files
- Can be used by unrelated processes
- Persist until explicitly deleted
- Support communication between processes that don't share a parent-child relationship

#### System Calls Demonstrated

**Writer**:
- `mkfifo()` - Create a named pipe (FIFO special file)
- `open()` - Open the pipe for writing
- `write()` - Send data through the pipe
- `close()` - Close the pipe

**Reader**:
- `open()` - Open the pipe for reading
- `read()` - Receive data from the pipe
- `close()` - Close the pipe

#### How to Run

**Important**: Named pipes have blocking behavior:
- Opening a pipe for reading blocks until a writer opens it
- Opening a pipe for writing blocks until a reader opens it
- This synchronization is a key feature of named pipes

**Compile**:
```bash
make
```

**Option 1: Run in two separate terminals**

Terminal 1 (Reader - start this first):
```bash
./named_pipe_reader
```

Terminal 2 (Writer):
```bash
./named_pipe_writer
```

**Option 2: Using Makefile targets**

Terminal 1:
```bash
make run-reader
```

Terminal 2:
```bash
make run-writer
```

**Option 3: Run writer in background**

```bash
./named_pipe_reader &
./named_pipe_writer
```

#### Expected Output

**Reader terminal**:
```
=== Named Pipe Reader ===

Opening named pipe 'my_named_pipe' for reading...
(This will block until a writer opens the pipe)

Named pipe opened (fd = 3)
Waiting for messages...

Received message 1 (37 bytes): Hello from the writer process!
Received message 2 (48 bytes): This is message #2 through the named pipe.
Received message 3 (33 bytes): Named pipes are great for IPC!
Received message 4 (27 bytes): This is the final message.

End of data (writer closed the pipe)

Closing named pipe...
Reader finished successfully!
Total messages received: 4
```

**Writer terminal**:
```
=== Named Pipe Writer ===

Creating named pipe 'my_named_pipe'...
Named pipe created successfully
Opening named pipe for writing...
(This will block until a reader opens the pipe)

Named pipe opened (fd = 3)
Starting to send messages...

Sent message 1: Hello from the writer process!
Sent message 2: This is message #2 through the named pipe.
Sent message 3: Named pipes are great for IPC!
Sent message 4: This is the final message.

Closing named pipe...
Writer finished successfully!

Note: The named pipe 'my_named_pipe' still exists in the filesystem.
You can remove it with: rm my_named_pipe
```

#### Understanding the FIFO File

After running the writer, you can see the named pipe in the filesystem:

```bash
ls -l my_named_pipe
```

Output:
```
prw-rw-rw- 1 user user 0 Nov 16 17:00 my_named_pipe
```

Note the `p` at the beginning, indicating it's a pipe (FIFO) special file.

## Tracing System Calls

You can use `strace` to see all system calls made by any program:

```bash
strace ./file_io_syscalls
strace ./named_pipe_writer
strace ./named_pipe_reader
```

This shows every system call with its arguments and return values.

## Cleaning Up

Remove compiled binaries and created files:

```bash
make clean
```

This removes:
- All compiled binaries
- `test_file.txt` (created by file_io_syscalls)
- `my_named_pipe` (created by named_pipe_writer)

## Compilation Details

All programs are compiled with:
- `-Wall` - Enable all warnings
- `-Wextra` - Enable extra warnings
- `-std=c99` - Use C99 standard

## Learning Resources

- `man 2 open` - Manual for open() system call
- `man 2 read` - Manual for read() system call
- `man 2 write` - Manual for write() system call
- `man 3 mkfifo` - Manual for mkfifo() function
- `man 7 fifo` - Manual for FIFO special files
