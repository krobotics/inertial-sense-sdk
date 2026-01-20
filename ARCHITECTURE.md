# Architecture Comparison: serial_port_t vs ISClient

## Old Architecture (serial_port_t)

```
┌─────────────────────────────────────────┐
│         User Application Code          │
│           (IMUStage class)              │
└───────────────┬─────────────────────────┘
                │
                │ Direct function calls
                ↓
┌─────────────────────────────────────────┐
│      serialPortPlatform.h/c             │
│   (Low-level serial port functions)     │
│                                         │
│  • serialPortPlatformInit()             │
│  • serialPortOpen()                     │
│  • serialPortWrite()                    │
│  • serialPortReadCharTimeout()          │
│  • serialPortClose()                    │
└───────────────┬─────────────────────────┘
                │
                │ Platform-specific
                ↓
┌─────────────────────────────────────────┐
│     serial_port_t structure             │
│   (Platform-specific serial port)       │
└───────────────┬─────────────────────────┘
                │
                ↓
        [Hardware Serial Port]
```

**Limitations:**
- Tightly coupled to serial port implementation
- Cannot easily switch to TCP or other transports
- Platform-specific code in application layer
- No abstraction for different connection types

---

## New Architecture (ISClient/ISStream)

```
┌─────────────────────────────────────────┐
│         User Application Code          │
│           (IMUStage class)              │
└───────────────┬─────────────────────────┘
                │
                │ Uses stream interface
                ↓
┌─────────────────────────────────────────┐
│          cISStream interface            │
│    (Abstract stream interface)          │
│                                         │
│  • Read()                               │
│  • Write()                              │
│  • Close()                              │
│  • Flush()                              │
└───────────────┬─────────────────────────┘
                │
                │ Polymorphic
                ↓
        ┌───────┴───────┐
        │               │
        ↓               ↓               ↓
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│cISSerialPort │ │ cISTcpClient │ │ cISZmqClient │
│              │ │              │ │              │
│ Wraps        │ │ Network      │ │ ZeroMQ       │
│ serial_port_t│ │ Socket       │ │ Socket       │
└──────┬───────┘ └──────┬───────┘ └──────┬───────┘
       │                │                │
       ↓                ↓                ↓
[Serial Port]     [TCP Socket]    [ZMQ Socket]


         Created via cISClient factory:
┌─────────────────────────────────────────┐
│        cISClient::                      │
│     OpenConnectionToServer()            │
│                                         │
│  Connection String Parser:              │
│  • "SERIAL:IS:port:baud" → cISSerialPort│
│  • "TCP:IS:ip:port"      → cISTcpClient │
│  • "ZMQ:IS:send:recv"    → cISZmqClient │
└─────────────────────────────────────────┘
```

**Benefits:**
- Abstracted communication interface
- Easy to switch between Serial/TCP/ZMQ
- Platform-specific code isolated in implementations
- Single API for all connection types
- Future-proof architecture
- Better testability (can mock cISStream)

---

## Connection String Format

The unified connection string makes it easy to switch transports:

```
TYPE:PROTOCOL:ADDRESS:PORT[:MOUNT][:USER][:PASS]
 │      │        │       │      └─ Optional
 │      │        │       └──────── Port/Baudrate
 │      │        └──────────────── Device/IP/Endpoint
 │      └───────────────────────── Data Format (IS/RTCM3/UBLOX)
 └──────────────────────────────── Transport (SERIAL/TCP/ZMQ)
```

### Examples:

**Serial Connection:**
```cpp
"SERIAL:IS:/dev/ttyACM0:921600"
"SERIAL:IS:COM3:115200"
```

**TCP Connection:**
```cpp
"TCP:IS:192.168.1.100:7777"
"TCP:RTCM3:192.168.1.100:2101"
```

**NTRIP Connection (TCP with authentication):**
```cpp
"TCP:RTCM3:192.168.1.100:2101:MOUNT:user:password"
```

**ZMQ Connection:**
```cpp
"ZMQ:IS:5556:5557"  // send_port:recv_port
```

---

## Data Flow Comparison

### Old Way (Direct Serial Port)
```
User Code
  └─ serialPortWrite(&serialPort, data, len)
     └─ Platform-specific write function
        └─ [Hardware]

User Code
  └─ serialPortReadCharTimeout(&serialPort, &byte, timeout)
     └─ Platform-specific read with timeout
        └─ [Hardware]
```

### New Way (Stream Interface)
```
User Code
  └─ stream->Write(data, len)
     └─ cISSerialPort::Write()
        └─ serialPortWrite() [internal]
           └─ [Hardware]

User Code
  └─ stream->Read(buffer, size)
     └─ cISSerialPort::Read()
        └─ serialPortRead() [internal]
           └─ [Hardware]
```

**Note:** The old serial port functions are still used internally by cISSerialPort,
but they're now hidden behind a clean interface.

---

## Migration Path

```
Step 1: Replace serial_port_t with cISStream*
  serial_port_t serialPort;  →  cISStream* stream;

Step 2: Change initialization
  serialPortPlatformInit(&serialPort);
  serialPortOpen(&serialPort, port, baud, blocking);
                    ↓
  stream = cISClient::OpenConnectionToServer("SERIAL:IS:" + port + ":921600");

Step 3: Update write calls
  serialPortWrite(&serialPort, buf, len);
                    ↓
  stream->Write(buf, len);

Step 4: Update read calls
  serialPortReadCharTimeout(&serialPort, &byte, timeout);
                    ↓
  uint8_t buffer[1];
  if (stream->Read(buffer, 1) > 0) {
      byte = buffer[0];
  }

Step 5: Add cleanup
  [automatic]  →  stream->Close();
                  delete stream;
```

---

## Code Reusability

With the new architecture, the same application code can work with any transport:

```cpp
// Same code, different transports!

// Serial device
stream = cISClient::OpenConnectionToServer("SERIAL:IS:/dev/ttyACM0:921600");

// Or TCP server
stream = cISClient::OpenConnectionToServer("TCP:IS:192.168.1.100:7777");

// Or ZMQ socket
stream = cISClient::OpenConnectionToServer("ZMQ:IS:5556:5557");

// Your application code remains the same:
stream->Write(data, len);
stream->Read(buffer, size);
```

This makes your code more flexible and easier to test!
