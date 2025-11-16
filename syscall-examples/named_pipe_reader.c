#include <fcntl.h>      // For open() flags
#include <unistd.h>     // For read(), close()
#include <stdio.h>      // For printf(), perror()
#include <errno.h>      // For errno

#define FIFO_NAME "my_named_pipe"
#define BUFFER_SIZE 256

int main() {
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int message_count = 0;

    printf("=== Named Pipe Reader ===\n\n");

    // 1. Open the named pipe for reading
    printf("Opening named pipe '%s' for reading...\n", FIFO_NAME);
    printf("(This will block until a writer opens the pipe)\n\n");

    fd = open(FIFO_NAME, O_RDONLY);
    if (fd == -1) {
        perror("Error opening named pipe for reading");
        printf("\nMake sure the writer has created the pipe first!\n");
        return 1;
    }

    printf("Named pipe opened (fd = %d)\n", fd);
    printf("Waiting for messages...\n\n");

    // 2. Read messages from the pipe until EOF
    while (1) {
        bytes_read = read(fd, buffer, BUFFER_SIZE - 1);

        if (bytes_read == -1) {
            perror("Error reading from named pipe");
            close(fd);
            return 1;
        }

        if (bytes_read == 0) {
            // EOF - writer has closed the pipe
            printf("\nEnd of data (writer closed the pipe)\n");
            break;
        }

        // Null-terminate and display the message
        buffer[bytes_read] = '\0';
        message_count++;
        printf("Received message %d (%zd bytes): %s", message_count, bytes_read, buffer);
    }

    // 3. Close the pipe
    printf("\nClosing named pipe...\n");
    if (close(fd) == -1) {
        perror("Error closing named pipe");
        return 1;
    }

    printf("Reader finished successfully!\n");
    printf("Total messages received: %d\n", message_count);

    return 0;
}
