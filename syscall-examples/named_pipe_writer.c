#include <fcntl.h>      // For open() flags
#include <sys/stat.h>   // For mkfifo() and mode bits
#include <unistd.h>     // For write(), close()
#include <stdio.h>      // For printf(), perror()
#include <string.h>     // For strlen()
#include <errno.h>      // For errno

#define FIFO_NAME "my_named_pipe"

int main() {
    int fd;
    char *messages[] = {
        "Hello from the writer process!\n",
        "This is message #2 through the named pipe.\n",
        "Named pipes are great for IPC!\n",
        "This is the final message.\n",
        NULL
    };
    int i = 0;

    printf("=== Named Pipe Writer ===\n\n");

    // 1. Create the named pipe (FIFO) using mkfifo() system call
    // Mode: 0666 (rw-rw-rw-)
    printf("Creating named pipe '%s'...\n", FIFO_NAME);
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        if (errno == EEXIST) {
            printf("Named pipe already exists, continuing...\n");
        } else {
            perror("Error creating named pipe");
            return 1;
        }
    } else {
        printf("Named pipe created successfully\n");
    }

    // 2. Open the named pipe for writing
    printf("Opening named pipe for writing...\n");
    printf("(This will block until a reader opens the pipe)\n\n");

    fd = open(FIFO_NAME, O_WRONLY);
    if (fd == -1) {
        perror("Error opening named pipe for writing");
        return 1;
    }

    printf("Named pipe opened (fd = %d)\n", fd);
    printf("Starting to send messages...\n\n");

    // 3. Write messages to the pipe
    while (messages[i] != NULL) {
        ssize_t bytes_written = write(fd, messages[i], strlen(messages[i]));

        if (bytes_written == -1) {
            perror("Error writing to named pipe");
            close(fd);
            return 1;
        }

        printf("Sent message %d: %s", i + 1, messages[i]);
        i++;

        // Small delay to make output more readable
        sleep(1);
    }

    // 4. Close the pipe
    printf("\nClosing named pipe...\n");
    if (close(fd) == -1) {
        perror("Error closing named pipe");
        return 1;
    }

    printf("Writer finished successfully!\n");
    printf("\nNote: The named pipe '%s' still exists in the filesystem.\n", FIFO_NAME);
    printf("You can remove it with: rm %s\n", FIFO_NAME);

    return 0;
}
