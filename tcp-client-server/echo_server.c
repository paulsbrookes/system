#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define BACKLOG 5
#define BUFFER_SIZE 1024

void log_message(const char *msg) {
    time_t now;
    time(&now);
    char *timestr = ctime(&now);
    timestr[strlen(timestr) - 1] = '\0';  // Remove newline
    printf("[%s] %s\n", timestr, msg);
    fflush(stdout);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    char log_buf[256];

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    log_message("Socket created");

    // Reuse address
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Setup address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    snprintf(log_buf, sizeof(log_buf), "Socket bound to port %d", PORT);
    log_message(log_buf);

    // Listen
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    snprintf(log_buf, sizeof(log_buf), "Server listening on port %d (backlog: %d)", PORT, BACKLOG);
    log_message(log_buf);
    log_message("Waiting for connections...");

    // Accept loop
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }

        snprintf(log_buf, sizeof(log_buf), "Client connected: %s:%d",
                 inet_ntoa(client_addr.sin_addr),
                 ntohs(client_addr.sin_port));
        log_message(log_buf);

        // Read and echo loop for this client
        ssize_t bytes_read;
        while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';

            snprintf(log_buf, sizeof(log_buf), "Received %zd bytes: \"%.100s%s\"",
                     bytes_read, buffer, bytes_read > 100 ? "..." : "");
            log_message(log_buf);

            // Echo back
            ssize_t bytes_written = write(client_fd, buffer, bytes_read);
            if (bytes_written > 0) {
                snprintf(log_buf, sizeof(log_buf), "Echoed %zd bytes back to client", bytes_written);
                log_message(log_buf);
            }
        }

        if (bytes_read == 0) {
            log_message("Client disconnected");
        } else if (bytes_read < 0) {
            perror("read failed");
        }

        close(client_fd);
    }

    close(server_fd);
    return 0;
}
