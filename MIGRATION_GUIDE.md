# Migration from serial_port_t to ISClient/ISStream

## Overview
This document describes the changes needed to migrate from using `serial_port_t` directly to using `cISClient` and `cISStream` for communication with Inertial Sense devices.

## Key Changes

### 1. Include Headers
**Before:**
```cpp
#include "InertialSense.h"
// Uses serialPortPlatform.h internally
```

**After:**
```cpp
#include "InertialSense.h"
// ISClient.h and ISStream.h are now included via InertialSense.h
```

### 2. Member Variable Declaration
**Before:**
```cpp
class IMUStage {
    serial_port_t serialPort;  // Low-level serial port structure
    ...
};
```

**After:**
```cpp
class IMUStage {
    static cISStream* stream;  // Stream interface (can be serial, TCP, or ZMQ)
    ...
};
```

Note: Made `stream` static to be accessible from the static `portWrite` function.

### 3. Port Write Function
**Before:**
```cpp
int portWrite(unsigned int port, const uint8_t *buf, int len) {
    return serialPortWrite(&IMUStage::serialPort, buf, len);
}
```

**After:**
```cpp
int portWrite(unsigned int port, const uint8_t *buf, int len) {
    return IMUStage::stream->Write(buf, len);
}
```

### 4. Function Signatures
**Before:**
```cpp
int IMUStage::set_configuration(serial_port_t *serialPort, is_comm_instance_t *comm);
int IMUStage::stop_message_broadcasting(serial_port_t *serialPort, is_comm_instance_t *comm);
int IMUStage::save_persistent_messages(serial_port_t *serialPort, is_comm_instance_t *comm);
int IMUStage::enable_message_broadcasting(serial_port_t *serialPort, is_comm_instance_t *comm);
```

**After:**
```cpp
int IMUStage::set_configuration(cISStream *stream, is_comm_instance_t *comm);
int IMUStage::stop_message_broadcasting(cISStream *stream, is_comm_instance_t *comm);
int IMUStage::save_persistent_messages(cISStream *stream, is_comm_instance_t *comm);
int IMUStage::enable_message_broadcasting(cISStream *stream, is_comm_instance_t *comm);
```

### 5. Opening the Connection
**Before:**
```cpp
// Initialize the serial port (Windows, MAC or Linux)
serialPortPlatformInit(&serialPort);
// Open serial, last parameter is a 1 which means a blocking read
if (!serialPortOpen(&serialPort, calib_.get()->imuSource.c_str(), IS_BAUDRATE_921600, 1)) {
    printf("Failed to serialPortOpen\n");
    return;
}
```

**After:**
```cpp
// Open stream using ISClient
// Format: "SERIAL:IS:port:baudrate"
std::string connectionString = "SERIAL:IS:" + calib_.get()->imuSource + ":921600";
stream = cISClient::OpenConnectionToServer(connectionString);
if (stream == nullptr) {
    printf("Failed to open stream connection\n");
    return;
}
```

### 6. Reading Data
**Before:**
```cpp
int count;
uint8_t inByte;
while ((!done_) && (count = serialPortReadCharTimeout(&serialPort, &inByte, 10000)) > 0) {
    // Process inByte
}
```

**After:**
```cpp
uint8_t inByte;
uint8_t readBuffer[1];
while (!done_) {
    int count = stream->Read(readBuffer, 1);
    if (count > 0) {
        inByte = readBuffer[0];
        // Process inByte
    } else if (count == 0) {
        // No data available, sleep briefly to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } else {
        // Error reading
        printf("Error reading from stream\n");
        break;
    }
}
```

### 7. Cleanup
**Before:**
```cpp
// Automatic cleanup when serial_port_t goes out of scope
```

**After:**
```cpp
// Cleanup: Close and delete the stream
if (stream) {
    stream->Close();
    delete stream;
    stream = nullptr;
}
```

## Benefits of Using ISClient/ISStream

1. **Unified Interface**: Same code works for serial ports, TCP connections, and ZMQ
2. **Connection String Format**: Easy to switch between connection types
3. **Better Abstraction**: Stream-based interface is more flexible
4. **Future-Proof**: New connection types can be added without changing application code

## Connection String Format

The connection string format is: `TYPE:PROTOCOL:ADDRESS:PORT`

Examples:
- Serial: `"SERIAL:IS:/dev/ttyACM0:921600"`
- TCP: `"TCP:IS:192.168.1.100:7777"`
- ZMQ: `"ZMQ:IS:5556:5557"`

Where:
- TYPE: SERIAL, TCP, or ZMQ
- PROTOCOL: IS (Inertial Sense), RTCM3, or UBLOX
- ADDRESS: Serial port path, IP address, or port numbers
- PORT: Baudrate for serial, port number for TCP, or receive port for ZMQ

## Important Notes

1. The `stream` member must be accessible from the `portWrite` function, so it's declared as static
2. ISStream doesn't have built-in per-read timeout like `serialPortReadCharTimeout`, so implement timeout logic in application code if needed
3. Always delete the stream object after closing to prevent memory leaks
4. The stream pointer should be checked for nullptr before use
5. Function parameters changed from `serial_port_t*` to `cISStream*` throughout the codebase
