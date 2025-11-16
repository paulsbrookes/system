# TCP Echo Server & Client

A simple TCP server and client written in C. The server echoes back any messages it receives, and both include detailed logging.

## What It Does

**Server (`echo_server`):**
- Listens on port 8080
- Accepts client connections
- Reads data from clients
- Echoes the data back to the client
- Logs all connections and data transfers with timestamps

**Client (`echo_client`):**
- Connects to the server on localhost:8080
- Sends messages (interactive mode or command-line argument)
- Displays echoed responses
- Logs all connection events and data transfers

## How to Use

### Compile

**Server:**
```bash
gcc -Wall -O2 echo_server.c -o echo_server
```

**Client:**
```bash
gcc -Wall -O2 echo_client.c -o echo_client
```

### Run the Server
```bash
./echo_server
```

The server will start and display:
```
[timestamp] Socket created
[timestamp] Socket bound to port 8080
[timestamp] Server listening on port 8080 (backlog: 5)
[timestamp] Waiting for connections...
```

### Send Messages to the Server

**Using netcat (recommended):**
```bash
nc localhost 8080
```

**Using telnet:**
```bash
telnet localhost 8080
```

**Using echo:**
```bash
echo "Hello, server!" | nc localhost 8080
```

Type your message and press Enter. The server will echo it back.

Press `Ctrl+C` to stop the server or client.

### Using the Custom Client

**Interactive mode:**
```bash
./echo_client
```

Then type messages at the prompt. Type `quit` or press `Ctrl+D` to exit.

**Single message mode:**
```bash
./echo_client Hello from the client!
```

Sends the message and exits immediately.

**Example session:**
```bash
# Terminal 1
./echo_server

# Terminal 2
./echo_client
> Hello, server!
[timestamp] Sent 15 bytes
[timestamp] Received 15 bytes
Echo: Hello, server!

> This is a test
[timestamp] Sent 15 bytes
[timestamp] Received 15 bytes
Echo: This is a test

> quit
```

## System Calls Used

**Server system calls:**
- **`socket()`** - Creates a socket endpoint for network communication
- **`setsockopt()`** - Sets socket options (SO_REUSEADDR to allow immediate port rebinding)
- **`bind()`** - Binds the socket to a specific address (INADDR_ANY) and port (8080)
- **`listen()`** - Marks the socket as passive, ready to accept incoming connections
- **`accept()`** - Blocks until a client connects, returns a new socket for that connection
- **`read()`** - Reads data from the client socket
- **`write()`** - Writes data back to the client socket (echoing)
- **`close()`** - Closes the socket connection

**Client system calls:**
- **`socket()`** - Creates a socket endpoint for network communication
- **`connect()`** - Initiates a connection to the server at localhost:8080
- **`write()`** - Sends data to the server
- **`read()`** - Reads echoed data from the server
- **`close()`** - Closes the socket connection

**Additional library functions:**
- `time()` / `ctime()` - For timestamp logging
- `inet_ntoa()` / `inet_pton()` - Converts between IP address formats
- `htons()` / `ntohs()` - Convert between host and network byte order

## What is TCP?

**TCP (Transmission Control Protocol)** is a connection-oriented, reliable network protocol:

- **Connection-oriented**: Requires establishing a connection before data transfer (via 3-way handshake)
- **Reliable**: Guarantees delivery of data in order, with error checking and retransmission
- **Byte stream**: Provides a continuous stream of bytes (no message boundaries)
- **Flow control**: Prevents overwhelming the receiver with too much data
- **Full-duplex**: Data can flow in both directions simultaneously

TCP is part of the TCP/IP protocol suite and operates at the Transport Layer (Layer 4) of the OSI model. It's used by protocols like HTTP, HTTPS, SSH, and FTP.

### TCP vs UDP

- **TCP**: Reliable, ordered, connection-based (like a phone call)
- **UDP**: Unreliable, connectionless, faster (like sending a postcard)
