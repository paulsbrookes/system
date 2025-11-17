# Network IP Address Display

A C program that displays all network interface IP addresses (both local and public) on your system.

## What It Does

This program demonstrates how to retrieve and display network interface information in C:

- **Local IP Addresses**: Lists all network interfaces (ethernet, WiFi, loopback, etc.) with their IPv4 and IPv6 addresses
- **Public IP Address**: Queries an external service to determine your public-facing IP address
- **Network Programming**: Shows basic HTTP client implementation and network interface enumeration

## How to Use

### Compile

```bash
gcc -Wall -O2 get_ip.c -o get_ip
```

### Run

```bash
./get_ip
```

### Example Output

```
╔════════════════════════════════════════════╗
║     Network IP Address Information        ║
╚════════════════════════════════════════════╝

=== Local Network Interfaces ===

Interface: lo          Family: IPv4    Address: 127.0.0.1
Interface: lo          Family: IPv6    Address: ::1
Interface: eth0        Family: IPv4    Address: 192.168.1.100
Interface: eth0        Family: IPv6    Address: fe80::a00:27ff:fe4e:66a1
Interface: wlan0       Family: IPv4    Address: 10.0.0.50
Interface: wlan0       Family: IPv6    Address: fe80::3e97:eff:fe12:3456

=== Public IP Address ===

Public IP: 203.0.113.42

Note: Local addresses are private to your network.
      Public address is visible on the internet.
```

## System Calls and Functions Used

### `getifaddrs(struct ifaddrs **ifap)`
**Purpose**: Retrieves a linked list of all network interfaces on the system.

- Creates a linked list containing information about each network interface
- Each entry includes interface name, address family, and network address
- Must be freed with `freeifaddrs()` when done
- Returns 0 on success, -1 on error

**Why it's useful**: This is the modern, portable way to enumerate network interfaces across different Unix-like systems.

### `getnameinfo()`
**Purpose**: Converts socket address structures to human-readable strings.

- Takes a `sockaddr` structure and produces numeric host and service strings
- `NI_NUMERICHOST` flag forces numeric IP address output (no DNS lookup)
- Works with both IPv4 (`AF_INET`) and IPv6 (`AF_INET6`)
- Returns 0 on success, non-zero error code on failure

**Why it's useful**: Provides a protocol-independent way to convert binary addresses to readable strings.

### `getaddrinfo()`
**Purpose**: Resolves hostnames and service names to socket addresses.

- Modern replacement for `gethostbyname()`
- Supports both IPv4 and IPv6
- Returns a linked list of address structures to try
- Protocol and address family independent

**Why it's useful**: Handles DNS resolution and provides addresses ready for `connect()`.

### `socket(int domain, int type, int protocol)`
**Purpose**: Creates a communication endpoint.

- `domain`: Address family (AF_INET for IPv4, AF_INET6 for IPv6)
- `type`: Socket type (SOCK_STREAM for TCP, SOCK_DGRAM for UDP)
- `protocol`: Usually 0 to select default protocol
- Returns file descriptor on success, -1 on error

### `connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)`
**Purpose**: Establishes a connection to a remote host.

- Initiates a connection on a socket
- For TCP, performs three-way handshake
- Blocks until connection established or error occurs
- Returns 0 on success, -1 on error

### `read(int fd, void *buf, size_t count)` / `write(int fd, const void *buf, size_t count)`
**Purpose**: Read from and write to file descriptors (including sockets).

- `read()`: Attempts to read up to count bytes into buffer
- `write()`: Attempts to write up to count bytes from buffer
- Return number of bytes actually transferred, or -1 on error
- May transfer fewer bytes than requested

### `freeifaddrs(struct ifaddrs *ifa)`
**Purpose**: Frees memory allocated by `getifaddrs()`.

- Releases entire linked list returned by `getifaddrs()`
- Must be called to prevent memory leaks

## Understanding Network Interfaces and IP Addresses

### Network Interfaces

A **network interface** is a point of connection between a computer and a network. Each interface can have one or more IP addresses assigned to it.

Common interface types:
- **lo** (loopback): Virtual interface for local communication (127.0.0.1)
- **eth0, eth1, ...**: Wired Ethernet connections
- **wlan0, wlan1, ...**: Wireless WiFi connections
- **docker0, veth...**: Virtual interfaces for containers
- **tun0, tap0**: VPN tunnel interfaces

### Local vs Public IP Addresses

**Local (Private) IP Addresses**:
- Used within your local network (home, office, etc.)
- Not routable on the public internet
- Common ranges:
  - `10.0.0.0/8` (10.0.0.0 - 10.255.255.255)
  - `172.16.0.0/12` (172.16.0.0 - 172.31.255.255)
  - `192.168.0.0/16` (192.168.0.0 - 192.168.255.255)
- Multiple devices can share the same private IP on different networks

**Public IP Address**:
- Assigned by your Internet Service Provider (ISP)
- Globally unique and routable on the internet
- This is the address external servers see when you connect
- Usually shared among multiple devices via Network Address Translation (NAT)

### IPv4 vs IPv6

**IPv4** (Internet Protocol version 4):
- 32-bit addresses (e.g., 192.168.1.100)
- Format: Four decimal numbers (0-255) separated by dots
- ~4.3 billion possible addresses
- The original and still most common protocol

**IPv6** (Internet Protocol version 6):
- 128-bit addresses (e.g., fe80::a00:27ff:fe4e:66a1)
- Format: Eight groups of four hexadecimal digits separated by colons
- 340 undecillion possible addresses
- Designed to solve IPv4 address exhaustion
- Supports abbreviated notation (consecutive zero groups can be replaced with ::)

### How the Public IP Retrieval Works

The program makes an HTTP request to `api.ipify.org`, a free service that returns your public IP address:

1. **DNS Resolution**: `getaddrinfo()` converts "api.ipify.org" to an IP address
2. **TCP Connection**: `connect()` establishes a connection to the server
3. **HTTP Request**: Sends a simple GET request
4. **Response Parsing**: Reads the response and extracts the IP address from the body

This demonstrates a basic HTTP client and shows how your machine appears to the outside world.

## Learning Points

1. **Network Interface Enumeration**: `getifaddrs()` provides a clean, portable way to discover network interfaces
2. **Protocol Independence**: Using `getnameinfo()` and `getaddrinfo()` makes code work with both IPv4 and IPv6
3. **Socket Programming Basics**: Creating sockets, connecting, and basic HTTP communication
4. **Memory Management**: Properly freeing allocated resources with `freeifaddrs()` and `freeaddrinfo()`
5. **Error Handling**: Checking return values and providing meaningful error messages
6. **Network Architecture**: Understanding the distinction between local and public addressing

## Troubleshooting

**"Could not resolve api.ipify.org"**
- Check your internet connection
- Verify DNS is working: `ping api.ipify.org`
- Firewall may be blocking DNS queries

**"Could not connect to api.ipify.org"**
- Internet connection may be down
- Firewall may be blocking outbound HTTP connections
- Try testing connectivity: `curl http://api.ipify.org`

**No network interfaces shown**
- Unlikely on most systems (even loopback should appear)
- Run as regular user (root not required)
- Check if `getifaddrs()` is supported on your system

## Next Steps

To extend this program, you could:

1. **Filter interfaces**: Add command-line options to show only specific interfaces
2. **Show more details**: Display netmask, broadcast address, MAC address
3. **IPv6 support**: Use an IPv6-compatible service for public IP
4. **JSON output**: Format output as JSON for use in scripts
5. **Interface statistics**: Show packet counts, errors, bandwidth using `/proc/net/dev`
6. **Multiple services**: Query multiple IP services and compare results
7. **Active connection check**: Determine which interface is used for internet traffic

## References

- `man 3 getifaddrs` - Network interface enumeration
- `man 3 getnameinfo` - Address-to-name translation
- `man 3 getaddrinfo` - Network address and service translation
- `man 2 socket` - Socket creation
- `man 2 connect` - Socket connection
- RFC 791 - Internet Protocol (IPv4)
- RFC 8200 - Internet Protocol, Version 6 (IPv6)
