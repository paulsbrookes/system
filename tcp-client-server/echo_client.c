#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

void log_message(const char *msg) {
    time_t now;
    time(&now);
    char *timestr = ctime(&now);
    timestr[strlen(timestr) - 1] = '\0';  // Remove newline
    printf("[%s] %s\n", timestr, msg);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char log_buf[256];
    ssize_t bytes_sent, bytes_received;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    log_message("Socket created");

    // Setup server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connect to server
    snprintf(log_buf, sizeof(log_buf), "Connecting to %s:%d...", SERVER_IP, PORT);
    log_message(log_buf);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    log_message("Connected to server");

    // If message provided as argument, send it and exit
    if (argc > 1) {
        // Join all arguments into one message
        char message[BUFFER_SIZE] = {0};
        for (int i = 1; i < argc; i++) {
            strcat(message, argv[i]);
            if (i < argc - 1) {
                strcat(message, " ");
            }
        }
        strcat(message, "\n");

        snprintf(log_buf, sizeof(log_buf), "Sending: \"%s\"", message);
        log_message(log_buf);

        bytes_sent = write(sock, message, strlen(message));
        if (bytes_sent < 0) {
            perror("write failed");
            close(sock);
            exit(EXIT_FAILURE);
        }

        snprintf(log_buf, sizeof(log_buf), "Sent %zd bytes", bytes_sent);
        log_message(log_buf);

        // Receive echo
        bytes_received = read(sock, buffer, sizeof(buffer) - 1);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            snprintf(log_buf, sizeof(log_buf), "Received echo (%zd bytes)", bytes_received);
            log_message(log_buf);
            printf("Echo: %s", buffer);
        }

        close(sock);
        log_message("Connection closed");
        return 0;
    }

    // Interactive mode
    log_message("Interactive mode - type messages (Ctrl+D or 'quit' to exit)");
    printf("\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("\n");
            break;  // EOF (Ctrl+D)
        }

        // Check for quit command
        if (strncmp(buffer, "quit", 4) == 0) {
            break;
        }

        // Send message to server
        bytes_sent = write(sock, buffer, strlen(buffer));
        if (bytes_sent < 0) {
            perror("write failed");
            break;
        }

        snprintf(log_buf, sizeof(log_buf), "Sent %zd bytes", bytes_sent);
        log_message(log_buf);

        // Receive echo
        bytes_received = read(sock, buffer, sizeof(buffer) - 1);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            snprintf(log_buf, sizeof(log_buf), "Received %zd bytes", bytes_received);
            log_message(log_buf);
            printf("Echo: %s\n", buffer);
        } else if (bytes_received == 0) {
            log_message("Server closed connection");
            break;
        } else {
            perror("read failed");
            break;
        }
    }

    close(sock);
    log_message("Connection closed");
    return 0;
}
