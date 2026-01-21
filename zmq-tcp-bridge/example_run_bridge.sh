#!/bin/bash

# Example script demonstrating ZMQ-to-TCP bridge usage
# This script shows how to start the bridge and connect an SDK client

echo "========================================="
echo "ZMQ-to-TCP Bridge Example"
echo "========================================="
echo ""

# Check if bridge executable exists
if [ ! -f "./zmq_tcp_bridge" ]; then
    echo "ERROR: zmq_tcp_bridge executable not found"
    echo "Please build the bridge first:"
    echo "  cd ../build"
    echo "  cmake .."
    echo "  make zmq_tcp_bridge"
    exit 1
fi

echo "Starting ZMQ-to-TCP bridge..."
echo "  ZMQ Receive: tcp://127.0.0.1:7115 (from IMU)"
echo "  ZMQ Send:    tcp://127.0.0.1:7116 (to IMU)"
echo "  TCP Port:    8000 (for SDK clients)"
echo ""
echo "The bridge will forward data between ZMQ and TCP."
echo ""
echo "To connect your InertialSense SDK client:"
echo "  is.OpenConnectionToServer(\"TCP:IS:127.0.0.1:8000\");"
echo ""
echo "Press Ctrl+C to stop the bridge."
echo ""
echo "========================================="
echo ""

# Start the bridge with default configuration
./zmq_tcp_bridge

# When bridge stops (Ctrl+C)
echo ""
echo "Bridge stopped."
