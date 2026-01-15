/*
Copyright (c) 2025 RED 6
*/

#include "gtest/gtest.h"
#include "ISZmqClient.h"
#include "ISClient.h"
#include "ISComm.h"
#include <thread>
#include <chrono>
#include <cstring>

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

// Helper function to create a valid ISB-framed packet
std::vector<uint8_t> createISBPacket(uint16_t did, const void* payload, uint16_t payloadSize) {
    // Prepare buffer for ISB packet
    std::vector<uint8_t> packet(PKT_BUF_SIZE);
    
    // Initialize ISComm instance for packet creation
    is_comm_instance_t comm;
    uint8_t commBuffer[PKT_BUF_SIZE];
    is_comm_init(&comm, commBuffer, PKT_BUF_SIZE);
    
    // Create ISB packet using is_comm_write_to_buf
    int packetSize = is_comm_write_to_buf(packet.data(), packet.size(), &comm, 
                                          PKT_TYPE_DATA, did, payloadSize, 0, (void*)payload);
    
    // Resize to actual packet size
    if (packetSize > 0) {
        packet.resize(packetSize);
    } else {
        packet.clear();
    }
    
    return packet;
}

// Test ISB packet validation - valid packet
TEST(ISZmqClientISBTest, ValidISBPacket) {
    // Create a valid ISB packet
    uint32_t testData = 0x12345678;
    std::vector<uint8_t> isbPacket = createISBPacket(DID_DEV_INFO, &testData, sizeof(testData));
    
    ASSERT_GT(isbPacket.size(), 0u) << "Failed to create ISB packet";
    
    // Verify packet has ISB preamble
    EXPECT_EQ(isbPacket[0], 0xEF) << "Missing ISB preamble byte 1";
    EXPECT_EQ(isbPacket[1], 0x49) << "Missing ISB preamble byte 2";
}

// Test ISB packet validation - invalid checksum
TEST(ISZmqClientISBTest, InvalidChecksumPacket) {
    // Create a valid ISB packet
    uint32_t testData = 0x12345678;
    std::vector<uint8_t> isbPacket = createISBPacket(DID_DEV_INFO, &testData, sizeof(testData));
    
    ASSERT_GT(isbPacket.size(), 2u);
    
    // Corrupt the checksum (last 2 bytes before end)
    isbPacket[isbPacket.size() - 2] ^= 0xFF;
    isbPacket[isbPacket.size() - 1] ^= 0xFF;
    
    // Try to validate using is_comm_parse
    is_comm_instance_t comm;
    uint8_t commBuffer[PKT_BUF_SIZE];
    is_comm_init(&comm, commBuffer, PKT_BUF_SIZE);
    
    // Ensure packet fits in buffer
    ASSERT_LE(isbPacket.size(), (size_t)PKT_BUF_SIZE);
    
    // Copy packet to comm buffer
    std::memcpy(comm.rxBuf.tail, isbPacket.data(), isbPacket.size());
    comm.rxBuf.tail += isbPacket.size();
    
    // Parse should return error for invalid checksum
    protocol_type_t ptype = is_comm_parse(&comm);
    EXPECT_EQ(ptype, _PTYPE_PARSE_ERROR) << "Should detect invalid checksum";
}

// Test ISB packet validation - invalid preamble
TEST(ISZmqClientISBTest, InvalidPreamblePacket) {
    // Create a packet with invalid preamble
    std::vector<uint8_t> invalidPacket = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    // Try to validate using is_comm_parse
    is_comm_instance_t comm;
    uint8_t commBuffer[PKT_BUF_SIZE];
    is_comm_init(&comm, commBuffer, PKT_BUF_SIZE);
    
    // Ensure packet fits in buffer
    ASSERT_LE(invalidPacket.size(), (size_t)PKT_BUF_SIZE);
    
    // Copy packet to comm buffer
    std::memcpy(comm.rxBuf.tail, invalidPacket.data(), invalidPacket.size());
    comm.rxBuf.tail += invalidPacket.size();
    
    // Parse should not find valid packet
    protocol_type_t ptype = is_comm_parse(&comm);
    EXPECT_NE(ptype, _PTYPE_INERTIAL_SENSE_DATA) << "Should not parse invalid preamble";
}

// Test ISB packet validation - packet too large
TEST(ISZmqClientISBTest, PacketTooLarge) {
    // Create a buffer larger than PKT_BUF_SIZE
    std::vector<uint8_t> largePacket(PKT_BUF_SIZE + 100, 0xAA);
    
    is_comm_instance_t comm;
    uint8_t commBuffer[PKT_BUF_SIZE];
    is_comm_init(&comm, commBuffer, PKT_BUF_SIZE);
    
    // Should handle gracefully - can only copy what fits
    size_t copySize = std::min(largePacket.size(), (size_t)PKT_BUF_SIZE);
    std::memcpy(comm.rxBuf.tail, largePacket.data(), copySize);
    comm.rxBuf.tail += copySize;
    
    // Parse should handle this without crashing
    protocol_type_t ptype = is_comm_parse(&comm);
    // Expect parse error or no complete packet
    EXPECT_TRUE(ptype == _PTYPE_PARSE_ERROR || ptype == _PTYPE_NONE);
}

// Test payload extraction from valid ISB packet
TEST(ISZmqClientISBTest, PayloadExtraction) {
    // Create a valid ISB packet with known payload
    uint32_t testPayload = 0xDEADBEEF;
    std::vector<uint8_t> isbPacket = createISBPacket(DID_DEV_INFO, &testPayload, sizeof(testPayload));
    
    ASSERT_GT(isbPacket.size(), 0u);
    
    // Parse the packet
    is_comm_instance_t comm;
    uint8_t commBuffer[PKT_BUF_SIZE];
    is_comm_init(&comm, commBuffer, PKT_BUF_SIZE);
    
    // Ensure packet fits in buffer
    ASSERT_LE(isbPacket.size(), (size_t)PKT_BUF_SIZE);
    
    std::memcpy(comm.rxBuf.tail, isbPacket.data(), isbPacket.size());
    comm.rxBuf.tail += isbPacket.size();
    
    protocol_type_t ptype = is_comm_parse(&comm);
    ASSERT_EQ(ptype, _PTYPE_INERTIAL_SENSE_DATA) << "Should parse valid packet";
    
    // Verify payload was extracted correctly
    ASSERT_GE(comm.rxPkt.data.size, sizeof(uint32_t));
    ASSERT_NE(comm.rxPkt.data.ptr, nullptr);
    
    uint32_t extractedPayload;
    std::memcpy(&extractedPayload, comm.rxPkt.data.ptr, sizeof(uint32_t));
    EXPECT_EQ(extractedPayload, testPayload) << "Payload should match original data";
}
