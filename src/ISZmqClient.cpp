/*
Copyright (c) 2025 RED 6
*/

#include "ISZmqClient.h"
#include "ISComm.h"
#include <zmq.hpp>
#include <cstring>

cISZmqClient::cISZmqClient()
    : m_context(nullptr)
    , m_sendSocket(nullptr)
    , m_recvSocket(nullptr)
    , m_isOpen(false)
    , m_commBuffer(PKT_BUF_SIZE)
{
    // Initialize ISComm instance for packet validation
    is_comm_init(&m_comm, m_commBuffer.data(), m_commBuffer.size());
}

cISZmqClient::~cISZmqClient()
{
    Close();
}

int cISZmqClient::Open(const std::string& sendEndpoint, const std::string& recvEndpoint)
{
    Close();

    try
    {
        // Create ZMQ context
        m_context = std::make_unique<zmq::context_t>(1);

        // Create send socket (PUB) for client_to_imu
        m_sendSocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::pub);
        m_sendSocket->connect(sendEndpoint);

        // Create receive socket (SUB) for imu_to_client
        m_recvSocket = std::make_unique<zmq::socket_t>(*m_context, zmq::socket_type::sub);
        m_recvSocket->connect(recvEndpoint);
        
        // Subscribe to all messages
        m_recvSocket->set(zmq::sockopt::subscribe, "");
        
        // Set non-blocking receive
        m_recvSocket->set(zmq::sockopt::rcvtimeo, 0);

        m_sendEndpoint = sendEndpoint;
        m_recvEndpoint = recvEndpoint;
        m_isOpen = true;

        return 0;
    }
    catch (const zmq::error_t& e)
    {
        // Cleanup on error
        m_sendSocket.reset();
        m_recvSocket.reset();
        m_context.reset();
        m_isOpen = false;
        return -1;
    }
}

int cISZmqClient::Close()
{
    if (m_isOpen)
    {
        try
        {
            if (m_sendSocket)
            {
                m_sendSocket->close();
                m_sendSocket.reset();
            }
            if (m_recvSocket)
            {
                m_recvSocket->close();
                m_recvSocket.reset();
            }
            if (m_context)
            {
                m_context->close();
                m_context.reset();
            }
        }
        catch (const zmq::error_t& e)
        {
            // Ignore errors during close
        }
        
        m_isOpen = false;
    }
    
    return 0;
}

int cISZmqClient::Read(void* data, int dataLength)
{
    if (!m_isOpen || !m_recvSocket || dataLength <= 0)
    {
        return -1;
    }

    try
    {
        zmq::message_t message;
        zmq::recv_result_t result = m_recvSocket->recv(message, zmq::recv_flags::dontwait);
        
        if (!result.has_value())
        {
            // No message available
            return 0;
        }

        size_t msgSize = message.size();
        if (msgSize == 0)
        {
            return 0;
        }

        // ZMQ messages contain ISB-framed packets - validate and decode them
        uint8_t* msgData = static_cast<uint8_t*>(message.data());
        
        // Reset comm buffer for parsing
        m_comm.rxBuf.head = m_comm.rxBuf.tail = m_comm.rxBuf.scan = m_comm.rxBuf.start;
        
        // Ensure message fits in available buffer space
        size_t availableSpace = m_comm.rxBuf.end - m_comm.rxBuf.tail;
        if (msgSize > availableSpace)
        {
            // Message too large for available buffer space
            return -1;
        }
        
        // Copy message to comm buffer
        std::memcpy(m_comm.rxBuf.tail, msgData, msgSize);
        m_comm.rxBuf.tail += msgSize;
        
        // Parse and validate the ISB packet
        protocol_type_t ptype = is_comm_parse(&m_comm);
        
        if (ptype == _PTYPE_PARSE_ERROR)
        {
            // Invalid packet - checksum failed or malformed
            return -1;
        }
        
        if (ptype == _PTYPE_NONE)
        {
            // No complete packet found (shouldn't happen with discrete ZMQ messages)
            return 0;
        }
        
        // Valid packet found - extract and return payload data (without ISB framing)
        if (ptype == _PTYPE_INERTIAL_SENSE_DATA || 
            ptype == _PTYPE_INERTIAL_SENSE_CMD || 
            ptype == _PTYPE_INERTIAL_SENSE_ACK)
        {
            // Get payload size (decoded, without ISB framing)
            size_t payloadSize = m_comm.rxPkt.data.size;
            
            if (payloadSize > (size_t)dataLength)
            {
                // Output buffer too small for decoded payload - signal error instead of truncating
                return -1;
            }
            
            if (payloadSize > 0 && m_comm.rxPkt.data.ptr != nullptr)
            {
                // Copy decoded payload data (actual data, not framed)
                std::memcpy(data, m_comm.rxPkt.data.ptr, payloadSize);
                return (int)payloadSize;
            }
        }
        
        // Unsupported packet type or no payload
        return 0;
    }
    catch (const zmq::error_t& e)
    {
        return -1;
    }
}

int cISZmqClient::Write(const void* data, int dataLength)
{
    if (!m_isOpen || !m_sendSocket || dataLength <= 0)
    {
        return -1;
    }

    try
    {
        // NOTE: Unlike Read(), Write() does NOT perform ISB framing or validation.
        // Callers MUST provide pre-framed ISB packets (e.g., via is_comm_write_to_buf()
        // or similar helpers) and pass the resulting buffer here. Write() then sends
        // the data as-is over ZMQ. This intentional asymmetry keeps this class as a
        // thin transport wrapper while higher layers handle packet construction.
        zmq::message_t message(dataLength);
        std::memcpy(message.data(), data, dataLength);
        
        zmq::send_result_t result = m_sendSocket->send(message, zmq::send_flags::dontwait);
        
        if (!result.has_value())
        {
            return 0;
        }

        return (int)result.value();
    }
    catch (const zmq::error_t& e)
    {
        return -1;
    }
}

std::string cISZmqClient::ConnectionInfo()
{
    if (m_isOpen)
    {
        return "ZMQ Send: " + m_sendEndpoint + ", Recv: " + m_recvEndpoint;
    }
    return "ZMQ (closed)";
}
