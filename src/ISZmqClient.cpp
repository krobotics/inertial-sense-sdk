/*
Copyright (c) 2025 RED 6
*/

#include "ISZmqClient.h"
#include <zmq.hpp>
#include <cstring>

cISZmqClient::cISZmqClient()
    : m_context(nullptr)
    , m_sendSocket(nullptr)
    , m_recvSocket(nullptr)
    , m_isOpen(false)
{
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
        if (msgSize > (size_t)dataLength)
        {
            // Message too large for buffer
            msgSize = (size_t)dataLength;
        }

        std::memcpy(data, message.data(), msgSize);
        return (int)msgSize;
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
