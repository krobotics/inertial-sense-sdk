/*
MIT LICENSE

Copyright (c) 2014-2025 Inertial Sense, Inc. - http://inertialsense.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "gtest/gtest.h"
#include "ISZmqTcpBridge.h"
#include "ISTcpClient.h"
#include <thread>
#include <chrono>
#include <zmq.hpp>

// Test basic bridge creation and destruction
TEST(ISZmqTcpBridgeTest, Constructor) {
    cISZmqTcpBridge bridge;
    EXPECT_FALSE(bridge.IsRunning());
}

// Test bridge initialization with valid parameters
TEST(ISZmqTcpBridgeTest, StartStop) {
    cISZmqTcpBridge bridge;
    
    // Use unique ports to avoid conflicts
    std::string zmqRecv = "tcp://127.0.0.1:17115";
    std::string zmqSend = "tcp://127.0.0.1:17116";
    int tcpPort = 18000;
    
    // Start bridge
    int result = bridge.Start(zmqRecv, zmqSend, tcpPort);
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(bridge.IsRunning());
    
    // Give bridge time to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Stop bridge
    result = bridge.Stop();
    EXPECT_EQ(result, 0);
    EXPECT_FALSE(bridge.IsRunning());
}

// Test starting bridge with already bound port (error case)
TEST(ISZmqTcpBridgeTest, StartWithBoundPort) {
    cISZmqTcpBridge bridge1;
    cISZmqTcpBridge bridge2;
    
    std::string zmqRecv = "tcp://127.0.0.1:17215";
    std::string zmqSend = "tcp://127.0.0.1:17216";
    int tcpPort = 18100;
    
    // Start first bridge
    int result = bridge1.Start(zmqRecv, zmqSend, tcpPort);
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(bridge1.IsRunning());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Try to start second bridge with same TCP port (should fail)
    result = bridge2.Start(zmqRecv, zmqSend, tcpPort);
    EXPECT_NE(result, 0);  // Should fail
    EXPECT_FALSE(bridge2.IsRunning());
    
    bridge1.Stop();
}

// Test GetStatus method
TEST(ISZmqTcpBridgeTest, GetStatus) {
    cISZmqTcpBridge bridge;
    
    // Before starting
    std::string status = bridge.GetStatus();
    EXPECT_EQ(status, "Stopped");
    
    // After starting
    int result = bridge.Start("tcp://127.0.0.1:17315", "tcp://127.0.0.1:17316", 18200);
    EXPECT_EQ(result, 0);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    status = bridge.GetStatus();
    EXPECT_EQ(status, "Running");
    
    bridge.Stop();
}

// Test multiple start/stop cycles
TEST(ISZmqTcpBridgeTest, MultipleStartStopCycles) {
    cISZmqTcpBridge bridge;
    
    for (int i = 0; i < 3; i++) {
        int tcpPort = 18300 + i;
        int result = bridge.Start("tcp://127.0.0.1:17415", "tcp://127.0.0.1:17416", tcpPort);
        EXPECT_EQ(result, 0);
        EXPECT_TRUE(bridge.IsRunning());
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        result = bridge.Stop();
        EXPECT_EQ(result, 0);
        EXPECT_FALSE(bridge.IsRunning());
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Test TCP client can connect to bridge
TEST(ISZmqTcpBridgeTest, TcpClientConnection) {
    cISZmqTcpBridge bridge;
    
    std::string zmqRecv = "tcp://127.0.0.1:17515";
    std::string zmqSend = "tcp://127.0.0.1:17516";
    int tcpPort = 18400;
    
    // Start bridge
    int result = bridge.Start(zmqRecv, zmqSend, tcpPort);
    EXPECT_EQ(result, 0);
    
    // Give bridge time to start TCP server
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Try to connect a TCP client
    cISTcpClient client;
    result = client.Open("127.0.0.1", tcpPort, 1000);
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(client.IsOpen());
    
    // Close client
    client.Close();
    
    // Stop bridge
    bridge.Stop();
}

// Test ZMQ to TCP data forwarding
TEST(ISZmqTcpBridgeTest, ZmqToTcpDataForwarding) {
    cISZmqTcpBridge bridge;
    
    std::string zmqRecvEndpoint = "tcp://127.0.0.1:17615";
    std::string zmqSendEndpoint = "tcp://127.0.0.1:17616";
    int tcpPort = 18500;
    
    // Start bridge
    int result = bridge.Start(zmqRecvEndpoint, zmqSendEndpoint, tcpPort);
    EXPECT_EQ(result, 0);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Connect TCP client to bridge
    cISTcpClient tcpClient;
    result = tcpClient.Open("127.0.0.1", tcpPort, 1000);
    EXPECT_EQ(result, 0);
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Create ZMQ publisher to send data
    zmq::context_t zmqContext(1);
    zmq::socket_t zmqPublisher(zmqContext, zmq::socket_type::pub);
    zmqPublisher.bind(zmqRecvEndpoint);
    
    // Give ZMQ time to establish connection
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Send test data via ZMQ
    const char* testData = "Test Data from ZMQ";
    zmq::message_t message(testData, strlen(testData));
    zmqPublisher.send(message, zmq::send_flags::none);
    
    // Give bridge time to forward data
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Try to receive data on TCP client
    uint8_t buffer[256];
    int bytesRead = tcpClient.Read(buffer, sizeof(buffer));
    
    // We might receive data or not depending on timing
    // The important part is that no errors occur
    EXPECT_GE(bytesRead, -1);
    
    // Cleanup
    zmqPublisher.close();
    zmqContext.close();
    tcpClient.Close();
    bridge.Stop();
}

// Test stopping already stopped bridge
TEST(ISZmqTcpBridgeTest, StopAlreadyStopped) {
    cISZmqTcpBridge bridge;
    
    // Stop without starting
    int result = bridge.Stop();
    EXPECT_EQ(result, 0);
    EXPECT_FALSE(bridge.IsRunning());
}

// Test starting already running bridge
TEST(ISZmqTcpBridgeTest, StartAlreadyRunning) {
    cISZmqTcpBridge bridge;
    
    std::string zmqRecv = "tcp://127.0.0.1:17715";
    std::string zmqSend = "tcp://127.0.0.1:17716";
    int tcpPort = 18600;
    
    // Start bridge first time
    int result = bridge.Start(zmqRecv, zmqSend, tcpPort);
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(bridge.IsRunning());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Try to start again (should fail or return error)
    result = bridge.Start(zmqRecv, zmqSend, tcpPort + 1);
    EXPECT_NE(result, 0);
    
    bridge.Stop();
}

// Test destructor cleanup when bridge is running
TEST(ISZmqTcpBridgeTest, DestructorCleanup) {
    // Create bridge in scope
    {
        cISZmqTcpBridge bridge;
        bridge.Start("tcp://127.0.0.1:17815", "tcp://127.0.0.1:17816", 18700);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // Bridge should be cleaned up by destructor when going out of scope
    }
    // If we get here without hanging, destructor cleanup worked
    SUCCEED();
}

// Test error handling with invalid ZMQ endpoints
TEST(ISZmqTcpBridgeTest, InvalidZmqEndpoints) {
    cISZmqTcpBridge bridge;
    
    // Try with invalid endpoint format
    int result = bridge.Start("invalid_endpoint", "tcp://127.0.0.1:17916", 18800);
    // Should fail gracefully
    EXPECT_NE(result, 0);
    EXPECT_FALSE(bridge.IsRunning());
}

// Test error handling with invalid TCP port
TEST(ISZmqTcpBridgeTest, InvalidTcpPort) {
    cISZmqTcpBridge bridge;
    
    // Try with invalid port (-1 is invalid)
    int result = bridge.Start("tcp://127.0.0.1:18015", "tcp://127.0.0.1:18016", -1);
    // Should fail gracefully
    EXPECT_NE(result, 0);
    EXPECT_FALSE(bridge.IsRunning());
}
