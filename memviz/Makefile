CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LIBS = -lncurses
TARGET = memviz
SRC = memviz.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

test: $(TARGET)
	@echo "Testing with current shell process (PID: $$$$)"
	./$(TARGET) $$$$

.PHONY: all clean test
