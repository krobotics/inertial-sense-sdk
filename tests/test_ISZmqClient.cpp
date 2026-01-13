/*
MIT LICENSE

Copyright (c) 2014-2025 Inertial Sense, Inc. - http://inertialsense.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "gtest/gtest.h"
#include "ISZmqClient.h"
#include "ISClient.h"
#include <thread>
#include <chrono>

// Test basic ZMQ client creation and destruction
TEST(ISZmqClientTest, Constructor) {
    cISZmqClient client;
    EXPECT_FALSE(client.IsOpen());
}

// Test endpoint macros
TEST(ISZmqClientTest, EndpointMacros) {
    std::string endpoint1 = ENDPOINT_HEADSET_1_IMU_TO_CLIENT;
    std::string endpoint2 = ENDPOINT_HEADSET_1_CLIENT_TO_IMU;
    std::string endpoint3 = ENDPOINT_HEADSET_2_IMU_TO_CLIENT;
    std::string endpoint4 = ENDPOINT_HEADSET_2_CLIENT_TO_IMU;
    
    EXPECT_EQ(endpoint1, "tcp://127.0.0.1:7115");
    EXPECT_EQ(endpoint2, "tcp://127.0.0.1:7116");
    EXPECT_EQ(endpoint3, "tcp://127.0.0.1:7135");
    EXPECT_EQ(endpoint4, "tcp://127.0.0.1:7136");
}

// Test Open with valid endpoints
TEST(ISZmqClientTest, OpenClose) {
    cISZmqClient client;
    
    // Open with valid endpoints
    int result = client.Open("tcp://127.0.0.1:15115", "tcp://127.0.0.1:15116");
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(client.IsOpen());
    
    // Close
    result = client.Close();
    EXPECT_EQ(result, 0);
    EXPECT_FALSE(client.IsOpen());
}

// Test ConnectionInfo
TEST(ISZmqClientTest, ConnectionInfo) {
    cISZmqClient client;
    
    // Before opening
    std::string info = client.ConnectionInfo();
    EXPECT_EQ(info, "ZMQ (closed)");
    
    // After opening
    client.Open("tcp://127.0.0.1:15115", "tcp://127.0.0.1:15116");
    info = client.ConnectionInfo();
    EXPECT_NE(info.find("tcp://127.0.0.1:15115"), std::string::npos);
    EXPECT_NE(info.find("tcp://127.0.0.1:15116"), std::string::npos);
    
    client.Close();
}

// Test ISClient with ZMQ connection string
TEST(ISZmqClientTest, ISClientOpenConnectionZMQ) {
    // Test ZMQ connection string format: ZMQ:IS:send_port:recv_port
    cISStream* stream = cISClient::OpenConnectionToServer("ZMQ:IS:15117:15118");
    EXPECT_NE(stream, nullptr);
    
    if (stream != nullptr) {
        EXPECT_NE(stream->ConnectionInfo().find("ZMQ"), std::string::npos);
        delete stream;
    }
}

// Test Read/Write operations (basic functionality)
TEST(ISZmqClientTest, ReadWriteOperations) {
    cISZmqClient client;
    
    // Open connection
    int result = client.Open("tcp://127.0.0.1:15119", "tcp://127.0.0.1:15120");
    ASSERT_EQ(result, 0);
    
    // Try to write data
    const char* testData = "Hello ZMQ";
    int written = client.Write(testData, strlen(testData));
    EXPECT_GE(written, 0);  // Should succeed or return 0 (no receiver)
    
    // Try to read data (should return 0 or -1 since there's no sender)
    char buffer[100];
    int read = client.Read(buffer, sizeof(buffer));
    EXPECT_GE(read, -1);  // Should return -1 (error) or 0 (no data)
    
    client.Close();
}

// Test Read with invalid parameters
TEST(ISZmqClientTest, ReadInvalidParameters) {
    cISZmqClient client;
    char buffer[100];
    
    // Read without opening
    int result = client.Read(buffer, sizeof(buffer));
    EXPECT_EQ(result, -1);
    
    // Open and try read with zero length
    client.Open("tcp://127.0.0.1:15121", "tcp://127.0.0.1:15122");
    result = client.Read(buffer, 0);
    EXPECT_EQ(result, -1);
    
    client.Close();
}

// Test Write with invalid parameters
TEST(ISZmqClientTest, WriteInvalidParameters) {
    cISZmqClient client;
    const char* testData = "Test";
    
    // Write without opening
    int result = client.Write(testData, strlen(testData));
    EXPECT_EQ(result, -1);
    
    // Open and try write with zero length
    client.Open("tcp://127.0.0.1:15123", "tcp://127.0.0.1:15124");
    result = client.Write(testData, 0);
    EXPECT_EQ(result, -1);
    
    client.Close();
}

// Test multiple Open/Close cycles
TEST(ISZmqClientTest, MultipleOpenClose) {
    cISZmqClient client;
    
    for (int i = 0; i < 3; i++) {
        int result = client.Open("tcp://127.0.0.1:15125", "tcp://127.0.0.1:15126");
        EXPECT_EQ(result, 0);
        EXPECT_TRUE(client.IsOpen());
        
        result = client.Close();
        EXPECT_EQ(result, 0);
        EXPECT_FALSE(client.IsOpen());
    }
}
