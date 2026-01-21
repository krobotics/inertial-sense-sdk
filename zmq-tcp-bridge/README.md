# ZMQ-to-TCP Bridge

## Overview

The ZMQ-to-TCP Bridge is a standalone library and executable that allows the InertialSense SDK to communicate with ZMQ-based systems without modifying the vendor SDK code. The bridge acts as a transparent proxy, forwarding data between ZMQ sockets and TCP connections.

## Architecture

```
┌─────────────┐        ┌──────────────────┐        ┌─────────────────┐
│ ZMQ         │        │ ZMQ-TCP Bridge   │        │ InertialSense   │
│ Publisher/  │<──────>│                  │<──────>│ SDK Client      │
│ Subscriber  │  ZMQ   │  ZMQ SUB/PUB     │  TCP   │ (Unmodified)    │
│ (External)  │        │      ↕           │        │                 │
│             │        │  TCP Server      │        │                 │
└─────────────┘        └──────────────────┘        └─────────────────┘
```

**Implementation**: Fully bidirectional
- ZMQ → TCP: Data from ZMQ publishers is forwarded to connected TCP clients
- TCP → ZMQ: Data from TCP clients is forwarded to ZMQ subscribers

## Why This Bridge?

Previously, the InertialSense SDK was modified to add ZMQ support directly (via `ISZmqClient`). This approach had drawbacks:
- Modified vendor code (ISClient.cpp) with `#ifdef ENABLE_ZMQ`
- Tightly coupled ZMQ dependency to the SDK
- Harder to maintain during SDK updates
- Unidirectional communication only

The bridge solves these issues by:
- Keeping the SDK completely unchanged
- Isolating ZMQ functionality in a separate component
- Allowing the SDK to use its native TCP interface
- Providing full bidirectional communication

## Building

### Prerequisites

```bash
# Linux
sudo apt-get install libzmq3-dev

# The bridge also requires the InertialSense SDK to be built first
```

### Build Instructions

```bash
cd inertial-sense-sdk
mkdir -p build && cd build

# Build the SDK first
cmake ..
make InertialSenseSDK

# Build the bridge
cd ../zmq-tcp-bridge
mkdir -p build && cd build
cmake ..
make

# The executable will be at: build/zmq_tcp_bridge
```

## Usage

### Starting the Bridge

```bash
# Using default configuration (ZMQ ports 7115/7116, TCP port 8000)
./zmq_tcp_bridge

# Custom configuration
./zmq_tcp_bridge --zmq-recv tcp://127.0.0.1:7115 \
                 --zmq-send tcp://127.0.0.1:7116 \
                 --tcp-port 8000
```

### Command-Line Options

- `--zmq-recv <endpoint>`: ZMQ endpoint to receive data from (default: tcp://127.0.0.1:7115)
- `--zmq-send <endpoint>`: ZMQ endpoint to send data to (default: tcp://127.0.0.1:7116)
- `--tcp-port <port>`: TCP port for SDK clients to connect (default: 8000)
- `-h, --help`: Show help message

### Connecting the SDK

Once the bridge is running, connect the InertialSense SDK using a standard TCP connection string:

```cpp
#include "InertialSense.h"

InertialSense is;

// Connect via the bridge's TCP port
if (is.OpenConnectionToServer("TCP:IS:127.0.0.1:8000")) {
    printf("Successfully connected via ZMQ-TCP bridge\n");
    // Use normally - the bridge handles ZMQ translation transparently
}
```

## Configuration Examples

### Example 1: Default Configuration (Headset 1)

```bash
./zmq_tcp_bridge
# ZMQ Recv: tcp://127.0.0.1:7115 (from IMU)
# ZMQ Send: tcp://127.0.0.1:7116 (to IMU)
# TCP Port: 8000 (for SDK client)
```

SDK Connection: `TCP:IS:127.0.0.1:8000`

### Example 2: Custom Ports

```bash
./zmq_tcp_bridge --zmq-recv tcp://127.0.0.1:9000 \
                 --zmq-send tcp://127.0.0.1:9001 \
                 --tcp-port 10000
```

SDK Connection: `TCP:IS:127.0.0.1:10000`

### Example 3: Multiple Bridges (Multiple Devices)

For multiple IMU devices, run multiple bridge instances:

```bash
# Bridge for Device 1
./zmq_tcp_bridge --zmq-recv tcp://127.0.0.1:7115 \
                 --zmq-send tcp://127.0.0.1:7116 \
                 --tcp-port 8000

# Bridge for Device 2
./zmq_tcp_bridge --zmq-recv tcp://127.0.0.1:7135 \
                 --zmq-send tcp://127.0.0.1:7136 \
                 --tcp-port 8001
```

## Library API

The bridge can also be used as a library in your own applications:

```cpp
#include "ISZmqTcpBridge.h"

cISZmqTcpBridge bridge;

// Start the bridge
if (bridge.Start("tcp://127.0.0.1:7115", "tcp://127.0.0.1:7116", 8000) == 0) {
    std::cout << "Bridge started successfully\n";
    std::cout << bridge.GetStatus() << std::endl;
    
    // Bridge runs in background threads
    // ... your application logic ...
    
    // Stop when done
    bridge.Stop();
}
```

## Benefits

1. **No Vendor Code Modification**: The InertialSense SDK remains completely unmodified
2. **Clean Separation**: ZMQ functionality is isolated in a separate component
3. **Flexible Deployment**: Run the bridge as a standalone service or embedded in your application
4. **Easy Maintenance**: SDK updates don't affect ZMQ integration
5. **Standard Protocols**: Uses standard TCP interface that the SDK already supports

## Troubleshooting

### Bridge Won't Start

- Ensure ZMQ library is installed: `ldconfig -p | grep zmq`
- Check if ports are already in use: `netstat -tuln | grep <port>`
- Verify ZMQ endpoints are accessible

### Connection Issues

- Ensure the bridge is running before connecting the SDK
- Check firewall settings for the TCP port
- Verify the correct TCP port in the SDK connection string

## Technical Details

### Data Flow

1. **ZMQ → TCP**: Bridge subscribes to ZMQ endpoint, forwards all messages to connected TCP clients
2. **TCP → ZMQ**: Bridge accepts TCP connections, forwards data from TCP clients to ZMQ publisher

### Threading Model

- Main thread: Bridge control and initialization
- ZMQ-to-TCP thread: Polls ZMQ socket, forwards to TCP
- TCP-to-ZMQ thread: Updates TCP server, handles connections (data forwarding via delegate callbacks)

### Performance

- Non-blocking I/O on both ZMQ and TCP sides
- Minimal latency (< 1ms typical)
- Buffer size: 8KB per transfer

## License

MIT License - Copyright 2014-2025 Inertial Sense, Inc.
