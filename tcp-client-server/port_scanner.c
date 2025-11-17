#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <time.h>

#define START_PORT 1
#define END_PORT 1024
#define DEFAULT_TARGET "192.168.59.21"
#define TIMEOUT_SEC 1
#define TIMEOUT_USEC 0

typedef enum {
    PORT_OPEN,
    PORT_CLOSED,
    PORT_FILTERED
} PortStatus;

typedef struct {
    int open;
    int closed;
    int filtered;
} ScanStats;

const char* get_service_name(int port) {
    switch(port) {
        case 20: return "ftp-data";
        case 21: return "ftp";
        case 22: return "ssh";
        case 23: return "telnet";
        case 25: return "smtp";
        case 53: return "dns";
        case 80: return "http";
        case 110: return "pop3";
        case 143: return "imap";
        case 443: return "https";
        case 445: return "smb";
        case 3306: return "mysql";
        case 3389: return "rdp";
        case 5432: return "postgresql";
        case 8080: return "http-alt";
        default: return "";
    }
}

PortStatus scan_port(const char *target_ip, int port) {
    int sock;
    struct sockaddr_in target_addr;
    int flags;
    PortStatus status = PORT_FILTERED;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return PORT_FILTERED;
    }

    // Set socket to non-blocking mode
    flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1 || fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        close(sock);
        return PORT_FILTERED;
    }

    // Setup target address
    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, target_ip, &target_addr.sin_addr) <= 0) {
        close(sock);
        return PORT_FILTERED;
    }

    // Attempt connection
    int result = connect(sock, (struct sockaddr *)&target_addr, sizeof(target_addr));

    if (result < 0) {
        if (errno == EINPROGRESS) {
            // Connection in progress, wait with timeout
            fd_set write_fds;
            struct timeval timeout;

            FD_ZERO(&write_fds);
            FD_SET(sock, &write_fds);

            timeout.tv_sec = TIMEOUT_SEC;
            timeout.tv_usec = TIMEOUT_USEC;

            result = select(sock + 1, NULL, &write_fds, NULL, &timeout);

            if (result > 0) {
                // Socket is writable, check for errors
                int error = 0;
                socklen_t len = sizeof(error);

                if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) == 0) {
                    if (error == 0) {
                        status = PORT_OPEN;
                    } else if (error == ECONNREFUSED) {
                        status = PORT_CLOSED;
                    } else {
                        status = PORT_FILTERED;
                    }
                }
            } else if (result == 0) {
                // Timeout
                status = PORT_FILTERED;
            } else {
                // Select error
                status = PORT_FILTERED;
            }
        } else if (errno == ECONNREFUSED) {
            status = PORT_CLOSED;
        } else {
            status = PORT_FILTERED;
        }
    } else {
        // Connection succeeded immediately
        status = PORT_OPEN;
    }

    close(sock);
    return status;
}

void print_status(const char *target_ip, int port, PortStatus status) {
    const char *service = get_service_name(port);
    const char *status_str;
    const char *color;

    switch(status) {
        case PORT_OPEN:
            status_str = "OPEN";
            color = "\033[0;32m";  // Green
            break;
        case PORT_CLOSED:
            status_str = "CLOSED";
            color = "\033[0;31m";  // Red
            break;
        case PORT_FILTERED:
            status_str = "FILTERED";
            color = "\033[0;33m";  // Yellow
            break;
        default:
            status_str = "UNKNOWN";
            color = "\033[0m";
            break;
    }

    if (strlen(service) > 0) {
        printf("%s%-6d %-10s %-15s\033[0m %s\n",
               color, port, status_str, service, target_ip);
    } else {
        printf("%s%-6d %-10s\033[0m %s\n",
               color, port, status_str, target_ip);
    }
    fflush(stdout);
}

void print_progress(int current, int total) {
    int percent = (current * 100) / total;
    int bar_width = 50;
    int filled = (current * bar_width) / total;

    printf("\rProgress: [");
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            printf("=");
        } else if (i == filled) {
            printf(">");
        } else {
            printf(" ");
        }
    }
    printf("] %d%% (%d/%d)", percent, current, total);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    const char *target_ip = DEFAULT_TARGET;
    ScanStats stats = {0, 0, 0};
    time_t start_time, end_time;
    int total_ports = END_PORT - START_PORT + 1;

    // Parse command-line arguments
    if (argc > 1) {
        target_ip = argv[1];
    }

    // Validate IP address
    struct in_addr addr;
    if (inet_pton(AF_INET, target_ip, &addr) <= 0) {
        fprintf(stderr, "Error: Invalid IP address: %s\n", target_ip);
        exit(EXIT_FAILURE);
    }

    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║           TCP Port Scanner - Sequential Scan               ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Target IP:    %s\n", target_ip);
    printf("Port range:   %d - %d\n", START_PORT, END_PORT);
    printf("Timeout:      %d second(s)\n", TIMEOUT_SEC);
    printf("Scan started: ");
    fflush(stdout);

    time(&start_time);
    printf("%s\n", ctime(&start_time));
    printf("────────────────────────────────────────────────────────────\n");
    printf("\n");

    // Scan all ports
    for (int port = START_PORT; port <= END_PORT; port++) {
        PortStatus status = scan_port(target_ip, port);

        // Update statistics
        switch(status) {
            case PORT_OPEN:
                stats.open++;
                print_status(target_ip, port, status);
                break;
            case PORT_CLOSED:
                stats.closed++;
                print_status(target_ip, port, status);
                break;
            case PORT_FILTERED:
                stats.filtered++;
                print_status(target_ip, port, status);
                break;
        }

        // Show progress every 10 ports (on a separate line below results)
        if (port % 10 == 0 || port == END_PORT) {
            printf("\n");
            print_progress(port - START_PORT + 1, total_ports);
            printf("\n\n");
        }
    }

    time(&end_time);
    double elapsed = difftime(end_time, start_time);

    // Print summary
    printf("\n");
    printf("════════════════════════════════════════════════════════════\n");
    printf("                      SCAN SUMMARY                          \n");
    printf("════════════════════════════════════════════════════════════\n");
    printf("Target:          %s\n", target_ip);
    printf("Ports scanned:   %d\n", total_ports);
    printf("\n");
    printf("\033[0;32mOpen ports:      %d\033[0m\n", stats.open);
    printf("\033[0;31mClosed ports:    %d\033[0m\n", stats.closed);
    printf("\033[0;33mFiltered ports:  %d\033[0m\n", stats.filtered);
    printf("\n");
    printf("Scan duration:   %.0f seconds\n", elapsed);
    printf("════════════════════════════════════════════════════════════\n");

    return 0;
}
