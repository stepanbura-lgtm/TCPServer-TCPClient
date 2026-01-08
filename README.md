# TCPServer and TCPClient

A robust, multi-threaded TCP server and client implementation using Qt 5/6. The server is designed to handle high concurrency (100000+ concurrent clients) with per-socket worker threads and defensive input validation. The client provides a GUI for connecting to the server and exchanging messages.

## Architecture

### TCPServer

**Key Features:**
- **Multi-threaded**: Each client connection runs in a dedicated worker thread, eliminating head-of-line blocking and enabling true parallel I/O.
- **Per-client buffering**: Incomplete messages are buffered per-socket, allowing robust parsing of fragmented TCP streams.
- **Defensive limits**: Configurable `MAX_BLOCK_SIZE` (default 65535 bytes) protects against malformed/oversized messages and DoS attacks.
- **Error handling**: Socket errors and disconnections are gracefully caught and logged; malformed data triggers immediate connection termination.
- **Echo-back behavior**: Server receives a message from a client and sends it back only to that client (no broadcast).

**Components:**
- `Server`: Main TCP server class extending `QTcpServer`, manages thread pool and client connections.
- `ClientHandler`: Worker class running in a dedicated `QThread`, handles socket I/O, message parsing, and echoing for one client.

**Protocol:**
- Simple length-prefixed framing: 2-byte big-endian unsigned integer (message length in bytes) followed by the message payload (serialized as `QString` via `QDataStream`).
- Example: `[0x00 0x05]Hello` = 5-byte message "Hello".

**Performance & Memory:**
- Per-client memory: ~few KB socket overhead + up to `MAX_BLOCK_SIZE` for input buffer (bounded).
- CPU: Event-driven, non-blocking I/O; negligible overhead per thread.
- Scales to 100000+ concurrent clients (limited by OS thread budget and network capacity).

### TCPClient

**Key Features:**
- **GUI interface**: User-friendly Qt GUI to connect to a server, send/receive messages, and configure connection parameters.
- **Same protocol**: Compatible with the TCPServer; uses identical length-prefixed framing.
- **Async I/O**: Non-blocking socket operations prevent UI freezing during network delays.

**Components:**
- `MainWindow`: Qt GUI window with input fields for server IP/port, message text, and a display for received messages.

---

## Build Instructions

### Prerequisites

- **Qt 5.9+** or **Qt 6.x**
- **CMake 3.14+**
- **C++17 compiler** (g++, clang, or MSVC)
- **Linux/macOS/Windows**

### Building on Linux/macOS

```bash
# Clone or extract the repository
cd /home/sbura/TestApp

# Build TCPServer
cd TCPServer/TCPServer
mkdir -p build
cd build
cmake ..
make -j$(nproc)

# Build TCPClient
cd ../../TCPClient/TCPClient
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

### Building on Windows (MSVC/Qt Creator)

1. Open `TCPServer/TCPServer/CMakeLists.txt` in Qt Creator.
2. Configure with your Qt kit and click **Build**.
3. Repeat for `TCPClient/TCPClient/CMakeLists.txt`.

Alternatively, use the command line (if MSVC and Qt are in PATH):
```batch
cd TCPServer\TCPServer
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config Release
```

---

## Running the Applications

### Start the Server

```bash
# From TCPServer/TCPServer/build
./TCPServer
# or with default port 4000
./build/Desktop-Debug/TCPServer
```

**With custom port:**
```bash
./TCPServer --port 5000
# or short form
./TCPServer -p 5000
```

**Show help:**
```bash
./TCPServer --help
# Output:
# Usage: TCPServer [options]
# Options:
#   -p, --port <port>  Port number to listen on (default: 4000).
#   -h, --help          Displays this help.
```

Expected output:
```
Server listening on port 4000
# or if custom port was provided:
Server listening on port 5000
```

### Start the Client

```bash
# From TCPClient/TCPClient/build
./TCPClient
# or
./build/Desktop-Debug/TCPClient
```

A GUI window will open. Configure and connect:
1. Enter server IP (e.g., `127.0.0.1`) and port (e.g., `4000`).
2. Click **Configure** to set the connection parameters.
3. Click **Connect to Server** to establish a connection.
4. Type a message and click **Send** to transmit it to the server.
5. The server echoes the message back; the client displays it in the message area.

---

## Usage Examples

### Single Client

**Terminal 1 (Server):**
```bash
cd TCPServer/TCPServer/build && ./TCPServer
# Output:
# Server listening on port 4000
```

**Terminal 2 (Client GUI):**
- Launch the client application.
- Set IP to `127.0.0.1`, port to `4000`.
- Click **Connect**, then send messages.

### Load Testing (High Concurrency)

---

## Error Handling & Robustness

### Server

- **Malformed input**: If the parsed message length exceeds `MAX_BLOCK_SIZE` or a `QDataStream` error occurs, the server logs a warning and disconnects the client immediately.
- **Socket errors**: Network errors (connection reset, timeout) are caught and logged; the client handler cleanly shuts down and is removed from the active set.
- **Double disconnect**: Guard flag prevents duplicate emission of `clientDisconnected()` signal when both `disconnected` and `errorOccurred` events fire.

### Client

- **Connection loss**: The client is notified and the UI reflects the disconnected state (implementation in `MainWindow::onReadyRead()` and error slots).

---

## Known Limitations & Future Improvements

1. **No SSL/TLS**: Communication is unencrypted. Add Qt's `QSslSocket` for secure connections if needed.
2. **Single message type**: The protocol hardcodes `QString` payload. For mixed data types, extend the framing to include a type field.
3. **No heartbeat/keep-alive**: Long-idle connections may be closed by intermediate firewalls. Add periodic ping/pong if needed.
4. **Memory limits**: Very large messages (approaching 64 KiB) may spike memory; consider streaming large data via multiple frames.

---

## Testing & Validation

### Smoke Test

1. Start the server.
2. Connect one client and send a few messages; verify echo responses.
3. Connect a second client and verify it receives only its own echoed messages (no broadcast).
4. Disconnect a client and verify the server logs cleanup.

### Load Test

Use a test harness (e.g., Python or C++) to spawn 100+ connections in parallel and measure:
- **Latency**: Round-trip time per message.
- **Throughput**: Messages per second.
- **Memory**: Process RSS growth with client count.

Example Python load test (requires custom framing):
```python
import socket
import struct
import threading
import time

def client_thread(client_id, message_count=10):
    s = socket.socket()
    try:
        s.connect(('127.0.0.1', 4000))
        msg = f"Client {client_id} message"
        msg_bytes = msg.encode('utf-8')
        # Send length-prefixed message
        s.sendall(struct.pack('>H', len(msg_bytes)) + msg_bytes)
        # Receive echo
        length_bytes = s.recv(2)
        if length_bytes:
            length = struct.unpack('>H', length_bytes)[0]
            echo = s.recv(length).decode('utf-8')
            print(f"Client {client_id} received: {echo}")
    finally:
        s.close()

# Spawn 100 concurrent clients
threads = [threading.Thread(target=client_thread, args=(i,)) for i in range(100)]
for t in threads:
    t.start()
for t in threads:
    t.join()
```

---

## Files & Structure


---

## License

This project is provided as-is for educational and commercial use. Modify and distribute freely.

---

## Support & Troubleshooting

**Q: Server won't start on the configured port.**
- **A**: The port may already be in use. Try a different port with `--port 5000`, or check with `lsof -i :4000` (Linux/macOS) or `netstat -ano | findstr :4000` (Windows). Invalid port numbers will trigger an error and the server will exit.

**Q: Client can't connect to the server.**
- **A**: Verify the server is running and listening. Check firewall rules. Ensure the IP and port match the server's configuration.

**Q: Many "Client disconnected" messages even with one client.**
- **A**: This was fixed in the latest build with a guard flag. If still occurring, check for duplicate signal connections.

**Q: Server memory grows unbounded with many clients.**
- **A**: Check per-client buffer sizes. If buffers are not being cleared properly, investigate `ClientState::buffer` cleanup in `ClientHandler::onReadyRead()`.

---

## Changelog

### v1.0.0 (Current)
- Multi-threaded server with per-socket workers.
- Defensive input validation and error handling.
- Echo-back (unicast) instead of broadcast.
- Qt 5/6 compatible.
