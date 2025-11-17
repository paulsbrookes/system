# TCP Port Scanner

A simple TCP port scanner written in C that scans ports on a target machine to discover which services are running. This tool uses sequential TCP connect scanning with non-blocking sockets and timeouts.

## What It Does

The port scanner attempts to connect to each port on a target IP address to determine its state:

- **OPEN** - Port accepts connections (service is running)
- **CLOSED** - Port actively refuses connections (no service listening)
- **FILTERED** - Connection times out (firewall blocking or no response)

## Features

- Scans ports 1-1024 (well-known and registered ports)
- Non-blocking socket connections with configurable timeout
- Color-coded output (green for OPEN, red for CLOSED, yellow for FILTERED)
- Recognizes common service names (HTTP, SSH, FTP, etc.)
- Real-time progress indicator
- Detailed scan statistics and summary
- Configurable target IP via command-line argument

## How to Use

### Compile

```bash
gcc -Wall -O2 port_scanner.c -o port_scanner
```

### Run

**Scan default target (192.168.59.21):**
```bash
./port_scanner
```

**Scan a specific IP address:**
```bash
./port_scanner 192.168.1.1
```

**Scan localhost:**
```bash
./port_scanner 127.0.0.1
```

### Example Output

```
╔════════════════════════════════════════════════════════════╗
║           TCP Port Scanner - Sequential Scan               ║
╚════════════════════════════════════════════════════════════╝

Target IP:    192.168.59.21
Port range:   1 - 1024
Timeout:      1 second(s)
Scan started: Sun Jan 15 14:32:01 2025

────────────────────────────────────────────────────────────

22     OPEN       ssh             192.168.59.21
80     OPEN       http            192.168.59.21
443    OPEN       https           192.168.59.21
8080   OPEN       http-alt        192.168.59.21

Progress: [==================================================] 100% (1024/1024)

════════════════════════════════════════════════════════════
                      SCAN SUMMARY
════════════════════════════════════════════════════════════
Target:          192.168.59.21
Ports scanned:   1024

Open ports:      4
Closed ports:    1020
Filtered ports:  0

Scan duration:   45 seconds
════════════════════════════════════════════════════════════
```

## System Calls Used

This program demonstrates several important system calls for network programming:

### Socket Management
- **`socket()`** - Creates a TCP socket for connection attempts
- **`close()`** - Closes the socket after each port check
- **`fcntl()`** - Sets socket to non-blocking mode (F_SETFL with O_NONBLOCK)
- **`setsockopt()`** / **`getsockopt()`** - Gets socket error status (SO_ERROR)

### Connection Handling
- **`connect()`** - Attempts to establish TCP connection to target port
  - Returns immediately in non-blocking mode with EINPROGRESS
  - Returns ECONNREFUSED if port is closed

### Timeout Implementation
- **`select()`** - Waits for socket to become writable with timeout
  - Monitors write file descriptor set
  - Returns when connection completes or timeout expires
  - Allows control over how long to wait per port

### Address Management
- **`inet_pton()`** - Converts IP address string to binary format
- **`htons()`** - Converts port number to network byte order

## How Port Scanning Works

### TCP Connect Scan

This scanner uses the **TCP connect scan** method, which is the most basic and reliable form of port scanning:

1. **Create socket** - A new TCP socket is created for each port
2. **Set non-blocking** - Socket is set to non-blocking mode to avoid hanging
3. **Attempt connection** - `connect()` is called to the target IP:port
4. **Wait with timeout** - `select()` waits up to 1 second for connection result
5. **Check result** - Socket error is checked via `getsockopt(SO_ERROR)`:
   - **error = 0** → Connection succeeded → Port is OPEN
   - **error = ECONNREFUSED** → Connection refused → Port is CLOSED
   - **timeout** → No response → Port is FILTERED (firewall)
6. **Close socket** - Socket is closed and next port is tested

### Port States Explained

| State | Meaning | Common Causes |
|-------|---------|---------------|
| **OPEN** | Service is listening and accepts connections | HTTP server, SSH daemon, database |
| **CLOSED** | Port is reachable but no service is listening | OS sends RST packet |
| **FILTERED** | No response received within timeout | Firewall blocking, host down, network issue |

## Configuration

You can modify these constants in the code:

```c
#define START_PORT 1        // First port to scan
#define END_PORT 1024       // Last port to scan
#define DEFAULT_TARGET "192.168.59.21"  // Default target IP
#define TIMEOUT_SEC 1       // Connection timeout in seconds
```

## Ethical and Legal Considerations

**⚠️ IMPORTANT - READ BEFORE USING ⚠️**

Port scanning can be considered a precursor to an attack and may be illegal in many jurisdictions without explicit authorization.

**Only scan systems you own or have explicit written permission to test.**

### Legal Use Cases:
- Scanning your own devices and networks
- Security audits with written authorization
- Educational purposes on isolated lab networks
- Penetration testing with signed agreements

### Illegal Activities:
- Scanning systems without permission
- Scanning to find vulnerabilities for exploitation
- Disrupting network services
- Unauthorized network reconnaissance

**This tool is provided for educational purposes only. The author is not responsible for any misuse.**

## What You Can Learn From This

This port scanner demonstrates:

1. **Network programming basics** - Socket creation, connection handling
2. **Non-blocking I/O** - How to prevent hanging on unresponsive hosts
3. **Timeout implementation** - Using `select()` for time-limited operations
4. **Error handling** - Distinguishing between different connection failures
5. **Real-world networking** - Understanding how services expose themselves
6. **System calls** - Practical use of low-level POSIX APIs

## Next Steps in Your Learning Journey

After mastering basic port scanning, consider:

1. **Multi-threaded scanning** - Use threads to scan multiple ports concurrently
2. **UDP scanning** - Detect UDP services (more challenging than TCP)
3. **Service fingerprinting** - Send protocol-specific probes to identify exact services
4. **SYN scanning** - Implement stealth scanning using raw sockets (requires root)
5. **Network mapper** - Scan entire subnets to discover active hosts
6. **Banner grabbing** - Connect and read service banners for version detection

## Troubleshooting

**Scan takes a long time:**
- Filtered ports wait for full timeout (1 second each)
- Reduce port range or increase TIMEOUT_SEC

**All ports show as FILTERED:**
- Target host may be down or blocking all traffic
- Check if target IP is reachable with `ping`
- Firewall may be dropping packets

**Permission denied errors:**
- Some systems restrict outbound connections
- Try running as root (not recommended for basic scanning)
- Check local firewall settings

**Scanning localhost shows unexpected OPEN ports:**
- This is normal! Check running services with `ss -tlnp` or `netstat -tlnp`
