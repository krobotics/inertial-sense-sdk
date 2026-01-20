# ISClient Migration Package

This directory contains comprehensive documentation and examples for migrating external code from using `serial_port_t` directly to using the modern `ISClient` and `ISStream` interface.

## 📁 Files in This Package

### 1. **SUMMARY.md** - Start Here! 🎯
Executive summary of the migration with quick-start instructions.
- What was requested
- What was delivered
- Key modifications at a glance
- Testing recommendations

### 2. **MIGRATION_GUIDE.md** - Step-by-Step Guide 📖
Comprehensive migration guide with detailed explanations.
- Side-by-side code comparisons
- Include header changes
- Member variable updates
- Function signature changes
- Connection opening/closing
- Data reading/writing
- Benefits of the new approach

### 3. **ARCHITECTURE.md** - Visual Design Overview 🏗️
Architecture comparison with ASCII diagrams.
- Old vs New architecture diagrams
- Data flow comparison
- Connection string format
- Code reusability examples
- Visual migration path

### 4. **IMUStage_original.h** - Reference Original Code 📄
Complete original code using `serial_port_t`.
- Shows the traditional approach
- Uses low-level serial port functions
- Platform-specific code

### 5. **IMUStage_updated.h** - Reference Updated Code ✨
Complete modernized code using `ISClient/ISStream`.
- Shows the new approach
- Uses abstract stream interface
- Platform-agnostic code

## 🚀 Quick Start

### For the Impatient Developer
1. Read **SUMMARY.md** (5 minutes)
2. Copy code from **IMUStage_updated.h**
3. Replace your old code
4. Test with your device

### For the Thorough Developer
1. Read **SUMMARY.md** to understand the scope
2. Study **ARCHITECTURE.md** to see the design changes
3. Follow **MIGRATION_GUIDE.md** step by step
4. Compare **IMUStage_original.h** with **IMUStage_updated.h**
5. Apply changes to your codebase incrementally
6. Test thoroughly

## 📋 What Changed?

### At a Glance

| Aspect | Before (serial_port_t) | After (ISClient) |
|--------|------------------------|------------------|
| **Include** | `serialPortPlatform.h` | `ISClient.h`, `ISStream.h` |
| **Type** | `serial_port_t` | `cISStream*` |
| **Init** | `serialPortPlatformInit()` | Not needed |
| **Open** | `serialPortOpen()` | `cISClient::OpenConnectionToServer()` |
| **Write** | `serialPortWrite()` | `stream->Write()` |
| **Read** | `serialPortReadCharTimeout()` | `stream->Read()` |
| **Close** | `serialPortClose()` | `stream->Close()` + `delete` |

### Connection Strings

The new approach uses connection strings for flexibility:

```cpp
// Serial (old way)
serialPortOpen(&port, "/dev/ttyACM0", 921600, 1);

// Serial (new way)
stream = cISClient::OpenConnectionToServer("SERIAL:IS:/dev/ttyACM0:921600");

// But now you can also do:
stream = cISClient::OpenConnectionToServer("TCP:IS:192.168.1.100:7777");
stream = cISClient::OpenConnectionToServer("ZMQ:IS:5556:5557");
// Same code, different transports!
```

## 🎯 Key Benefits

1. **Unified Interface**: One API for Serial, TCP, and ZMQ
2. **Easy Testing**: Can mock the stream interface
3. **Future-Proof**: New transports can be added without changing your code
4. **Better Abstraction**: Platform-specific details are hidden
5. **More Flexible**: Switch transports with just a connection string change

## ⚠️ Important Notes

### Memory Management
```cpp
// Always clean up!
if (stream) {
    stream->Close();
    delete stream;
    stream = nullptr;
}
```

### Static Members
The `stream` member needs to be static if accessed from static functions like `portWrite`:
```cpp
class YourClass {
    static cISStream* stream;  // Static for access from portWrite
};
```

### Timeout Handling
ISStream doesn't have built-in per-read timeouts like `serialPortReadCharTimeout`. Implement your own:
```cpp
// Old way (built-in timeout)
serialPortReadCharTimeout(&port, &byte, 10000);  // 10 second timeout

// New way (manual timeout handling)
uint8_t buffer[1];
int count = stream->Read(buffer, 1);
if (count == 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
```

## 🔍 Code Review Checklist

Before finalizing your migration, check:

- [ ] All `serial_port_t` references replaced with `cISStream*`
- [ ] `serialPortPlatformInit()` calls removed
- [ ] `serialPortOpen()` replaced with `OpenConnectionToServer()`
- [ ] `serialPortWrite()` replaced with `stream->Write()`
- [ ] `serialPortRead*()` replaced with `stream->Read()`
- [ ] Added null checks for stream pointer
- [ ] Added cleanup code (Close + delete)
- [ ] Function signatures updated to use `cISStream*`
- [ ] Static portWrite function can access stream
- [ ] Connection string format is correct
- [ ] Error handling is in place
- [ ] Tested with actual hardware

## 📚 Additional Resources

### In the InertialSense SDK:
- `src/ISClient.h` - Client factory class
- `src/ISStream.h` - Base stream interface
- `src/ISSerialPort.h` - Serial port implementation
- `src/ISTcpClient.h` - TCP client implementation
- `ExampleProjects/ISComm/` - Example usage

### Online:
- InertialSense Support: support@inertialsense.com
- SDK Documentation: https://github.com/inertialsense/inertial-sense-sdk

## 🐛 Troubleshooting

### "Stream is nullptr after OpenConnectionToServer"
- Check connection string format: `"SERIAL:IS:port:baudrate"`
- Verify port name is correct (e.g., `/dev/ttyACM0` or `COM3`)
- Ensure device is connected and accessible
- Check permissions on Linux (`sudo usermod -a -G dialout $USER`)

### "Undefined reference to IMUStage::stream"
- Add definition in your .cpp file: `cISStream* IMUStage::stream = nullptr;`
- Initialize in constructor: `stream = nullptr;`

### "portWrite cannot access non-static member"
- Make stream static: `static cISStream* stream;`
- Or make portWrite non-static and pass instance

### "Read returns 0 constantly"
- This is normal for non-blocking reads
- Add small sleep to avoid busy-waiting
- Check that device is broadcasting data

## 📞 Getting Help

1. Read through all documentation files first
2. Compare your code with the example files
3. Check the troubleshooting section
4. Contact InertialSense support if still stuck

## 📄 License

These migration documents and examples are provided as part of the InertialSense SDK.

MIT License - Copyright (c) 2014-2026 Inertial Sense, Inc.

---

**Good luck with your migration! 🚀**

If you find any issues or have suggestions, please file an issue on the InertialSense SDK GitHub repository.
