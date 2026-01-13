# ZMQ Interface Usage

The Inertial Sense SDK now supports ZMQ (ZeroMQ) as an interface to the sensor, in addition to the existing Serial and TCP interfaces.

## Overview

The ZMQ interface provides bidirectional communication using two separate sockets:
- **client_to_imu**: For sending data from client to IMU
- **imu_to_client**: For receiving data from IMU to client

## Connection String Format

To use the ZMQ interface, use the following connection string format:

```
ZMQ:IS:<send_port>:<recv_port>
```

Where:
- `send_port`: The port for client_to_imu communication
- `recv_port`: The port for imu_to_client communication

## Predefined Endpoints

The SDK provides predefined endpoint macros for two IMU devices:

```cpp
ENDPOINT_HEADSET_1_IMU_TO_CLIENT           // tcp://127.0.0.1:7115
ENDPOINT_HEADSET_1_CLIENT_TO_IMU           // tcp://127.0.0.1:7116
ENDPOINT_HEADSET_2_IMU_TO_CLIENT           // tcp://127.0.0.1:7135
ENDPOINT_HEADSET_2_CLIENT_TO_IMU           // tcp://127.0.0.1:7136
```

## Usage Examples

### Example 1: Using predefined ports for Headset 1

```cpp
#include "InertialSense.h"

InertialSense is;

// Connect to Headset 1
// Send on port 7116, receive on port 7115
if (is.OpenConnectionToServer("ZMQ:IS:7116:7115")) {
    printf("Successfully connected via ZMQ\n");
    // Use the connection...
}
```

### Example 2: Using custom ports

```cpp
#include "InertialSense.h"

InertialSense is;

// Connect using custom ports
if (is.OpenConnectionToServer("ZMQ:IS:8000:8001")) {
    printf("Successfully connected via ZMQ on custom ports\n");
    // Use the connection...
}
```

### Example 3: Direct ISZmqClient usage

```cpp
#include "ISZmqClient.h"

cISZmqClient client;

// Open connection with explicit endpoints
if (client.Open("tcp://127.0.0.1:7116", "tcp://127.0.0.1:7115") == 0) {
    // Write data
    const char* data = "Hello IMU";
    client.Write(data, strlen(data));
    
    // Read data
    char buffer[1024];
    int bytesRead = client.Read(buffer, sizeof(buffer));
    
    client.Close();
}
```

## Requirements

### Linux
Install the ZMQ development library:
```bash
sudo apt-get install libzmq3-dev
```

### Windows
Install ZMQ via vcpkg or download from https://zeromq.org/

## Implementation Details

The ZMQ interface uses:
- **PUB/SUB sockets** for reliable messaging
- **Non-blocking I/O** for Read operations
- **Header-only C++ bindings** (zmq.hpp from cppzmq)

The implementation follows the same pattern as existing interfaces (ISSerialPort and ISTcpClient), inheriting from cISStream for consistency.

## Testing

Run the ZMQ interface tests:
```bash
cd tests/build
./IS-SDK_unit-tests --gtest_filter="ISZmqClientTest.*"
```
