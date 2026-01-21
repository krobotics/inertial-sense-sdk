# Inertial Sense SDK with ZMQ-TCP Bridge

This repository contains the ZMQ-TCP bridge for the Inertial Sense SDK. The InertialSense SDK is included as a git submodule.

## Prerequisites

### Linux

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    libzmq3-dev \
    libusb-1.0-0-dev \
    libudev-dev
```

### Windows

- Visual Studio 2019 or later with C++ support
- CMake 3.10 or later
- ZMQ library (libzmq)

## Building

### Clone the Repository

When cloning this repository, make sure to initialize submodules recursively:

```bash
git clone --recurse-submodules https://github.com/krobotics/inertial-sense-sdk.git
cd inertial-sense-sdk
```

If you already cloned without `--recurse-submodules`, initialize them manually:

```bash
git submodule update --init --recursive
```

### Build Instructions

#### Linux

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

The build will produce:
- `inertialsense/libInertialSenseSDK.a` - Static library for the InertialSense SDK
- `zmq-tcp-bridge/libZMQTCPBridge.so` - Shared library for the ZMQ-TCP bridge
- `zmq-tcp-bridge/zmq_tcp_bridge` - Standalone bridge executable

#### Windows

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## ZMQ-TCP Bridge

The ZMQ-TCP bridge forwards data between ZMQ sockets and TCP connections, allowing the InertialSense SDK to receive data via TCP without requiring ZMQ support in the SDK itself.

### Usage

```bash
./build/zmq-tcp-bridge/zmq_tcp_bridge [OPTIONS]
```

**Options:**
- `--zmq-recv <endpoint>` - ZMQ endpoint to receive from (default: tcp://127.0.0.1:7115)
- `--zmq-send <endpoint>` - ZMQ endpoint to send to (default: tcp://127.0.0.1:7116)
- `--tcp-port <port>` - TCP port for SDK clients (default: 8000)
- `-h, --help` - Show help message

**Example:**

```bash
# Start bridge with default settings
./build/zmq-tcp-bridge/zmq_tcp_bridge

# Start bridge with custom ports
./build/zmq-tcp-bridge/zmq_tcp_bridge --zmq-recv tcp://127.0.0.1:7115 --tcp-port 9000
```

## Architecture

```
ZMQ Publisher (external) → ZMQ SUB socket → Bridge → TCP Server → SDK Client
```

The bridge receives data from ZMQ publishers and forwards it to TCP clients connected to the bridge's TCP server. This allows legacy applications that use TCP to work with ZMQ-based data sources.

## Troubleshooting

### ZMQ library not found

If CMake cannot find the ZMQ library, you'll see a warning:

```
ZMQ library not found. ZMQ-TCP bridge will not be available.
```

To fix this, install libzmq:

```bash
# Linux
sudo apt-get install libzmq3-dev

# Or build from source
git clone https://github.com/zeromq/libzmq.git
cd libzmq
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### Submodule not initialized

If you see build errors about missing files, make sure submodules are initialized:

```bash
git submodule update --init --recursive
```

## License

MIT License - See the LICENSE file in each submodule for details.

## Support

For issues related to:
- **ZMQ Bridge**: File an issue in this repository
- **InertialSense SDK**: Visit https://github.com/inertialsense/inertial-sense-sdk
