/*
 * get_ip.c - Display Network Interface IP Addresses
 *
 * This program displays IP addresses for all network interfaces on the system,
 * including both IPv4 and IPv6 addresses. It also attempts to retrieve the
 * public IP address by querying an external service.
 *
 * Compilation: gcc -Wall -O2 get_ip.c -o get_ip
 * Usage: ./get_ip
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PUBLIC_IP_HOST "api.ipify.org"
#define PUBLIC_IP_PORT "80"
#define BUFFER_SIZE 1024

/*
 * display_local_ips - Display all local network interface IP addresses
 *
 * Uses getifaddrs() to retrieve a list of all network interfaces and their
 * associated addresses. Displays both IPv4 and IPv6 addresses.
 */
void display_local_ips(void) {
    struct ifaddrs *ifaddr, *ifa;
    int family;
    char host[NI_MAXHOST];

    printf("=== Local Network Interfaces ===\n\n");

    /* Get linked list of network interfaces */
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    /* Iterate through each interface */
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }

        family = ifa->ifa_addr->sa_family;

        /* Process IPv4 and IPv6 addresses only */
        if (family == AF_INET || family == AF_INET6) {
            /* Convert address to human-readable string */
            int s = getnameinfo(ifa->ifa_addr,
                               (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                     sizeof(struct sockaddr_in6),
                               host, NI_MAXHOST,
                               NULL, 0, NI_NUMERICHOST);

            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                continue;
            }

            printf("Interface: %-10s  Family: %-6s  Address: %s\n",
                   ifa->ifa_name,
                   (family == AF_INET) ? "IPv4" : "IPv6",
                   host);
        }
    }

    freeifaddrs(ifaddr);
    printf("\n");
}

/*
 * get_public_ip - Retrieve public IP address from external service
 *
 * Connects to api.ipify.org via HTTP to retrieve the public-facing IP address.
 * This demonstrates basic HTTP client functionality and shows the difference
 * between local and public IP addresses.
 */
void get_public_ip(void) {
    struct addrinfo hints, *res, *rp;
    int sockfd, s;
    char buffer[BUFFER_SIZE];
    ssize_t n;

    printf("=== Public IP Address ===\n\n");

    /* Set up hints for getaddrinfo */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* TCP socket */

    /* Resolve hostname */
    s = getaddrinfo(PUBLIC_IP_HOST, PUBLIC_IP_PORT, &hints, &res);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        printf("Could not resolve %s\n\n", PUBLIC_IP_HOST);
        return;
    }

    /* Try each address until we successfully connect */
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) {
            continue;
        }

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break; /* Success */
        }

        close(sockfd);
    }

    freeaddrinfo(res);

    if (rp == NULL) {
        fprintf(stderr, "Could not connect to %s\n\n", PUBLIC_IP_HOST);
        return;
    }

    /* Send HTTP GET request */
    const char *request = "GET / HTTP/1.1\r\n"
                         "Host: api.ipify.org\r\n"
                         "Connection: close\r\n"
                         "\r\n";

    if (write(sockfd, request, strlen(request)) == -1) {
        perror("write");
        close(sockfd);
        return;
    }

    /* Read response */
    memset(buffer, 0, BUFFER_SIZE);
    n = read(sockfd, buffer, BUFFER_SIZE - 1);
    if (n > 0) {
        /* Parse HTTP response to extract IP address */
        char *body = strstr(buffer, "\r\n\r\n");
        if (body != NULL) {
            body += 4; /* Skip past "\r\n\r\n" */
            printf("Public IP: %s\n", body);
        } else {
            printf("Could not parse response\n");
        }
    } else if (n == -1) {
        perror("read");
    }

    close(sockfd);
    printf("\n");
}

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════╗\n");
    printf("║     Network IP Address Information        ║\n");
    printf("╚════════════════════════════════════════════╝\n");
    printf("\n");

    /* Display all local network interface addresses */
    display_local_ips();

    /* Retrieve and display public IP address */
    get_public_ip();

    printf("Note: Local addresses are private to your network.\n");
    printf("      Public address is visible on the internet.\n");
    printf("\n");

    return 0;
}
