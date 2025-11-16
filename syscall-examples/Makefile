CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGETS = file_io_syscalls named_pipe_writer named_pipe_reader
FIFO_NAME = my_named_pipe

all: $(TARGETS)

file_io_syscalls: file_io_syscalls.c
	$(CC) $(CFLAGS) -o file_io_syscalls file_io_syscalls.c

named_pipe_writer: named_pipe_writer.c
	$(CC) $(CFLAGS) -o named_pipe_writer named_pipe_writer.c

named_pipe_reader: named_pipe_reader.c
	$(CC) $(CFLAGS) -o named_pipe_reader named_pipe_reader.c

clean:
	rm -f $(TARGETS) test_file.txt $(FIFO_NAME)

run-file-io: file_io_syscalls
	./file_io_syscalls

# Note: Run reader and writer in separate terminals
run-writer: named_pipe_writer
	./named_pipe_writer

run-reader: named_pipe_reader
	./named_pipe_reader

.PHONY: all clean run-file-io run-writer run-reader
