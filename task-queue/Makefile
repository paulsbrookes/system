# Makefile for Task Queue Examples
# Builds three progressive examples of task queue implementations

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
CFLAGS_PTHREAD = -Wall -Wextra -std=gnu99 -O2
PTHREAD_FLAGS = -pthread

# Binaries
SIMPLE = 01_simple_queue
THREADSAFE = 02_threadsafe_queue
THREADPOOL = 03_threadpool

# Source files
SIMPLE_SRC = 01_simple_queue.c
THREADSAFE_SRC = 02_threadsafe_queue.c
THREADPOOL_SRC = 03_threadpool.c

# Default target: build all examples
all: $(SIMPLE) $(THREADSAFE) $(THREADPOOL)

# Build simple single-threaded queue
$(SIMPLE): $(SIMPLE_SRC)
	$(CC) $(CFLAGS) $(SIMPLE_SRC) -o $(SIMPLE)

# Build thread-safe queue (requires pthread)
$(THREADSAFE): $(THREADSAFE_SRC)
	$(CC) $(CFLAGS_PTHREAD) $(PTHREAD_FLAGS) $(THREADSAFE_SRC) -o $(THREADSAFE)

# Build thread pool (requires pthread)
$(THREADPOOL): $(THREADPOOL_SRC)
	$(CC) $(CFLAGS_PTHREAD) $(PTHREAD_FLAGS) $(THREADPOOL_SRC) -o $(THREADPOOL)

# Run the simple queue example
run-simple: $(SIMPLE)
	./$(SIMPLE)

# Run the thread-safe queue example
run-threadsafe: $(THREADSAFE)
	./$(THREADSAFE)

# Run the thread pool example
run-threadpool: $(THREADPOOL)
	./$(THREADPOOL)

# Run all examples in sequence
run-all: all
	@echo "=== Running Simple Queue ==="
	@./$(SIMPLE)
	@echo ""
	@echo "=== Running Thread-Safe Queue ==="
	@./$(THREADSAFE)
	@echo ""
	@echo "=== Running Thread Pool ==="
	@./$(THREADPOOL)

# Clean up compiled binaries
clean:
	rm -f $(SIMPLE) $(THREADSAFE) $(THREADPOOL)

# Help target
help:
	@echo "Task Queue Examples - Build Targets:"
	@echo "  make           - Build all examples"
	@echo "  make all       - Build all examples"
	@echo "  make clean     - Remove compiled binaries"
	@echo ""
	@echo "Individual builds:"
	@echo "  make 01_simple_queue      - Build simple queue only"
	@echo "  make 02_threadsafe_queue  - Build thread-safe queue only"
	@echo "  make 03_threadpool        - Build thread pool only"
	@echo ""
	@echo "Run examples:"
	@echo "  make run-simple      - Run simple queue"
	@echo "  make run-threadsafe  - Run thread-safe queue"
	@echo "  make run-threadpool  - Run thread pool"
	@echo "  make run-all         - Run all examples in sequence"

.PHONY: all clean run-simple run-threadsafe run-threadpool run-all help
