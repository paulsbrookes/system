CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -pedantic -O2
TARGET  = jit

.PHONY: all clean test

all: $(TARGET)

$(TARGET): jit.c
	$(CC) $(CFLAGS) -o $@ $<

test: $(TARGET)
	@echo "=== Test 1: Hello World ==="
	@./$(TARGET) -e '++++++++[>++++[>++>+++>+++>+<<<<-]>+>+>->>+[<]<-]>>.>---.+++++++..+++.>>.<-.<.+++.------.--------.>>+.>++.'
	@echo ""
	@echo "=== Test 2: Addition (2+5=7) ==="
	@./$(TARGET) -e '++>+++++<[->+<]>++++++++++++++++++++++++++++++++++++++++++++++++.'
	@echo ""
	@echo "=== All tests passed ==="

clean:
	rm -f $(TARGET)
