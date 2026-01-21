# ZMQ Interface Usage

The Inertial Sense SDK supports ZMQ (ZeroMQ) as an interface to the sensor through a standalone ZMQ-to-TCP bridge. This approach keeps the vendor SDK code unchanged while providing ZMQ compatibility.

## Architecture

The ZMQ interface is implemented using a **bridge architecture**:

```
┌─────────────┐        ┌──────────────────┐        ┌─────────────────┐
│ ZMQ         │        │ ZMQ-TCP Bridge   │        │ InertialSense   │
│ Publisher   │───────>│                  │───────>│ SDK Client      │
│ (External)  │  ZMQ   │  ZMQ SUB → TCP   │  TCP   │ (Unmodified)    │
└─────────────┘        │                  │        └─────────────────┘
                       └──────────────────┘
```

The bridge acts as a transparent proxy:
- Subscribes to ZMQ endpoints for incoming IMU data
- Provides a TCP server for SDK clients
- Forwards data between ZMQ and TCP transparently

**Benefits:**
- No modifications to InertialSense vendor code
- Easy to maintain during SDK updates
- ZMQ functionality isolated in separate component
- Standard TCP interface remains unchanged

## Quick Start

### 1. Build the Bridge

```bash
cd inertial-sense-sdk/build
cmake ..
make zmq_tcp_bridge
```

### 2. Start the Bridge

```bash
# Using default configuration (ZMQ ports 7115/7116, TCP port 8000)
./zmq-tcp-bridge/zmq_tcp_bridge

# Or with custom ports
./zmq-tcp-bridge/zmq_tcp_bridge --zmq-recv tcp://127.0.0.1:7115 \
                                 --zmq-send tcp://127.0.0.1:7116 \
                                 --tcp-port 8000
```

### 3. Connect SDK Client

```cpp
#include "InertialSense.h"

InertialSense is;

// Connect via TCP to the bridge (NOT directly via ZMQ)
if (is.OpenConnectionToServer("TCP:IS:127.0.0.1:8000")) {
    printf("Successfully connected via ZMQ-TCP bridge\n");
    // Use normally - bridge handles ZMQ translation
}
```

## Predefined Endpoints

The bridge supports standard ZMQ endpoints for IMU devices:

```
ENDPOINT_HEADSET_1_IMU_TO_CLIENT    tcp://127.0.0.1:7115
ENDPOINT_HEADSET_1_CLIENT_TO_IMU    tcp://127.0.0.1:7116
ENDPOINT_HEADSET_2_IMU_TO_CLIENT    tcp://127.0.0.1:7135
ENDPOINT_HEADSET_2_CLIENT_TO_IMU    tcp://127.0.0.1:7136
```

## Usage Examples

### Example 1: Default Configuration (Headset 1)

```bash
# Start bridge
./zmq_tcp_bridge

# In your application
is.OpenConnectionToServer("TCP:IS:127.0.0.1:8000");
```

### Example 2: Multiple Devices

Run separate bridge instances for each device:

```bash
# Bridge for Device 1
./zmq_tcp_bridge --zmq-recv tcp://127.0.0.1:7115 \
                 --zmq-send tcp://127.0.0.1:7116 \
                 --tcp-port 8000 &

# Bridge for Device 2
./zmq_tcp_bridge --zmq-recv tcp://127.0.0.1:7135 \
                 --zmq-send tcp://127.0.0.1:7136 \
                 --tcp-port 8001 &

# In your application
is1.OpenConnectionToServer("TCP:IS:127.0.0.1:8000");  // Device 1
is2.OpenConnectionToServer("TCP:IS:127.0.0.1:8001");  // Device 2
```

### Example 3: Using the Bridge Library

You can also embed the bridge in your own applications:

```cpp
#include "ISZmqTcpBridge.h"

// Create and start bridge
cISZmqTcpBridge bridge;
bridge.Start("tcp://127.0.0.1:7115", "tcp://127.0.0.1:7116", 8000);

// Bridge runs in background threads
// Your application can now connect via TCP...

// Stop when done
bridge.Stop();
```

## Requirements

### Linux
```bash
sudo apt-get install libzmq3-dev
```

### Windows
Install ZMQ via vcpkg or download from https://zeromq.org/

## Bridge Implementation Details

The bridge uses:
- **PUB/SUB ZMQ sockets** for ZMQ communication
- **TCP server** for SDK client connections
- **Non-blocking I/O** for minimal latency
- **Multi-threaded** design for concurrent ZMQ and TCP handling

See `zmq-tcp-bridge/README.md` for detailed documentation.

## Migrating from Direct ZMQ Integration

If you previously used direct ZMQ connection strings like `"ZMQ:IS:7116:7115"`, you now need to:

1. Start the bridge with those ZMQ endpoints
2. Change your connection string to TCP pointing to the bridge's TCP port

**Before:**
```cpp
is.OpenConnectionToServer("ZMQ:IS:7116:7115");  // No longer supported
```

**After:**
```bash
# Terminal 1: Start bridge
./zmq_tcp_bridge --zmq-recv tcp://127.0.0.1:7115 \
                 --zmq-send tcp://127.0.0.1:7116 \
                 --tcp-port 8000
```
```cpp
// In your code
is.OpenConnectionToServer("TCP:IS:127.0.0.1:8000");  // Connect via bridge
```

## Testing

The bridge can be tested without actual ZMQ publishers by using ZMQ test tools or mockups. For integration testing with the SDK:

```bash
# Terminal 1: Start your ZMQ data source
python zmq_publisher.py --port 7115

# Terminal 2: Start the bridge
./zmq_tcp_bridge

# Terminal 3: Run your SDK application
./your_app  # connects to TCP port 8000
```

## Troubleshooting

### Bridge Won't Start
- Ensure ZMQ library is installed: `ldconfig -p | grep zmq`
- Check if ports are in use: `netstat -tuln | grep <port>`

### Can't Connect from SDK
- Verify bridge is running: Check console output
- Test TCP port: `telnet 127.0.0.1 8000`
- Check firewall settings

### No Data Flow
- Verify ZMQ publisher is sending data
- Check ZMQ endpoint addresses match
- Enable debug logging in bridge (modify source if needed)

## Advanced Topics

For advanced usage including:
- Bidirectional TCP→ZMQ forwarding
- Custom data transformation
- Performance tuning
- Multiple simultaneous connections

See the detailed documentation in `zmq-tcp-bridge/README.md`.
