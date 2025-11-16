#include <fcntl.h>      // For open() flags
#include <unistd.h>     // For read(), write(), close()
#include <stdio.h>      // For printf(), perror()
#include <string.h>     // For strlen()

#define FILENAME "test_file.txt"
#define BUFFER_SIZE 256

int main() {
    int fd;
    ssize_t bytes_written, bytes_read;
    char write_buffer[] = "Hello, System Calls! This is written using the write() syscall.\n";
    char read_buffer[BUFFER_SIZE];

    printf("=== Writing to file using system calls ===\n\n");

    // 1. WRITE: Open file for writing (create if doesn't exist)
    // Flags: O_CREAT (create if not exists), O_WRONLY (write-only), O_TRUNC (truncate to 0)
    // Mode: 0644 (rw-r--r--)
    fd = open(FILENAME, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Error opening file for writing");
        return 1;
    }

    printf("File opened for writing (fd = %d)\n", fd);

    // 2. Write data to the file
    bytes_written = write(fd, write_buffer, strlen(write_buffer));
    if (bytes_written == -1) {
        perror("Error writing to file");
        close(fd);
        return 1;
    }

    printf("Wrote %zd bytes: %s\n", bytes_written, write_buffer);

    // 3. Close the file descriptor
    if (close(fd) == -1) {
        perror("Error closing file after writing");
        return 1;
    }

    printf("File closed after writing\n\n");

    printf("=== Reading from file using system calls ===\n\n");

    // 4. READ: Open file for reading
    fd = open(FILENAME, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file for reading");
        return 1;
    }

    printf("File opened for reading (fd = %d)\n", fd);

    // 5. Read data from the file
    bytes_read = read(fd, read_buffer, BUFFER_SIZE - 1);
    if (bytes_read == -1) {
        perror("Error reading from file");
        close(fd);
        return 1;
    }

    // Null-terminate the string
    read_buffer[bytes_read] = '\0';

    printf("Read %zd bytes: %s\n", bytes_read, read_buffer);

    // 6. Close the file descriptor
    if (close(fd) == -1) {
        perror("Error closing file after reading");
        return 1;
    }

    printf("File closed after reading\n\n");
    printf("=== System call demonstration complete ===\n");

    return 0;
}
