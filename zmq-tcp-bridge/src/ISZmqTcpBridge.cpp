/*
MIT LICENSE

Copyright (c) 2014-2025 Inertial Sense, Inc. - http://inertialsense.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ISZmqTcpBridge.h"
#include "ISTcpServer.h"
#include <zmq.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <errno.h>

cISZmqTcpBridge::cISZmqTcpBridge()
    : m_zmqContext(nullptr)
    , m_zmqRecvSocket(nullptr)
    , m_zmqSendSocket(nullptr)
    , m_tcpServer(nullptr)
    , m_zmqToTcpThread(nullptr)
    , m_tcpToZmqThread(nullptr)
    , m_isRunning(false)
    , m_tcpPort(0)
{
}

cISZmqTcpBridge::~cISZmqTcpBridge()
{
    Stop();
}

int cISZmqTcpBridge::Start(const std::string& zmqRecvEndpoint, const std::string& zmqSendEndpoint, int tcpPort)
{
    if (m_isRunning)
    {
        std::cerr << "Bridge is already running" << std::endl;
        return -1;
    }

    try
    {
        // Create ZMQ context
        m_zmqContext = std::make_unique<zmq::context_t>(1);

        // Create ZMQ receive socket (SUB) for receiving data from ZMQ publisher
        m_zmqRecvSocket = std::make_unique<zmq::socket_t>(*m_zmqContext, zmq::socket_type::sub);
        m_zmqRecvSocket->connect(zmqRecvEndpoint);
        m_zmqRecvSocket->set(zmq::sockopt::subscribe, "");  // Subscribe to all messages
        m_zmqRecvSocket->set(zmq::sockopt::rcvtimeo, 100);  // 100ms timeout for polling

        // Create ZMQ send socket (PUB) for sending data to ZMQ subscriber
        m_zmqSendSocket = std::make_unique<zmq::socket_t>(*m_zmqContext, zmq::socket_type::pub);
        m_zmqSendSocket->connect(zmqSendEndpoint);

        // Create TCP server
        m_tcpServer = std::make_unique<cISTcpServer>(nullptr);
        if (m_tcpServer->Open("", tcpPort) != 0)
        {
            std::cerr << "Failed to open TCP server on port " << tcpPort << std::endl;
            // Clean up ZMQ resources
            m_zmqRecvSocket->close();
            m_zmqSendSocket->close();
            m_zmqRecvSocket.reset();
            m_zmqSendSocket.reset();
            m_zmqContext->close();
            m_zmqContext.reset();
            m_tcpServer.reset();
            return -1;
        }

        // Store configuration
        m_zmqRecvEndpoint = zmqRecvEndpoint;
        m_zmqSendEndpoint = zmqSendEndpoint;
        m_tcpPort = tcpPort;

        // Set running flag before starting forwarding threads
        m_isRunning = true;

        // Start forwarding threads
        m_zmqToTcpThread = std::make_unique<std::thread>(&cISZmqTcpBridge::ZmqToTcpForwardingThread, this);
        m_tcpToZmqThread = std::make_unique<std::thread>(&cISZmqTcpBridge::TcpToZmqForwardingThread, this);

        std::cout << "ZMQ-to-TCP Bridge started:" << std::endl;
        std::cout << "  ZMQ Recv: " << zmqRecvEndpoint << std::endl;
        std::cout << "  ZMQ Send: " << zmqSendEndpoint << std::endl;
        std::cout << "  TCP Port: " << tcpPort << std::endl;

        return 0;
    }
    catch (const zmq::error_t& e)
    {
        std::cerr << "ZMQ error: " << e.what() << std::endl;

        // Ensure any partially initialized resources are cleaned up.
        m_isRunning = false;

        // Join any threads that may have been started.
        if (m_zmqToTcpThread && m_zmqToTcpThread->joinable())
        {
            m_zmqToTcpThread->join();
        }
        if (m_tcpToZmqThread && m_tcpToZmqThread->joinable())
        {
            m_tcpToZmqThread->join();
        }

        // Clean up resources similar to Stop(), but without relying on m_isRunning.
        try
        {
            if (m_tcpServer)
            {
                m_tcpServer->Close();
                m_tcpServer.reset();
            }

            if (m_zmqRecvSocket)
            {
                m_zmqRecvSocket->close();
                m_zmqRecvSocket.reset();
            }

            if (m_zmqSendSocket)
            {
                m_zmqSendSocket->close();
                m_zmqSendSocket.reset();
            }

            if (m_zmqContext)
            {
                m_zmqContext->close();
                m_zmqContext.reset();
            }
        }
        catch (const zmq::error_t& cleanupError)
        {
            std::cerr << "Error during cleanup after ZMQ error: " << cleanupError.what() << std::endl;
        }

        m_zmqToTcpThread.reset();
        m_tcpToZmqThread.reset();

        return -1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error starting bridge: " << e.what() << std::endl;

        // Ensure any partially initialized resources are cleaned up.
        m_isRunning = false;

        // Join any threads that may have been started.
        if (m_zmqToTcpThread && m_zmqToTcpThread->joinable())
        {
            m_zmqToTcpThread->join();
        }
        if (m_tcpToZmqThread && m_tcpToZmqThread->joinable())
        {
            m_tcpToZmqThread->join();
        }

        // Clean up resources similar to Stop(), but without relying on m_isRunning.
        try
        {
            if (m_tcpServer)
            {
                m_tcpServer->Close();
                m_tcpServer.reset();
            }

            if (m_zmqRecvSocket)
            {
                m_zmqRecvSocket->close();
                m_zmqRecvSocket.reset();
            }

            if (m_zmqSendSocket)
            {
                m_zmqSendSocket->close();
                m_zmqSendSocket.reset();
            }

            if (m_zmqContext)
            {
                m_zmqContext->close();
                m_zmqContext.reset();
            }
        }
        catch (const zmq::error_t& cleanupError)
        {
            std::cerr << "Error during cleanup after start failure: " << cleanupError.what() << std::endl;
        }

        m_zmqToTcpThread.reset();
        m_tcpToZmqThread.reset();
        return -1;
    }
}

int cISZmqTcpBridge::Stop()
{
    if (!m_isRunning)
    {
        return 0;
    }

    std::cout << "Stopping ZMQ-to-TCP Bridge..." << std::endl;

    // Signal threads to stop
    m_isRunning = false;

    // Wait for threads to finish
    if (m_zmqToTcpThread && m_zmqToTcpThread->joinable())
    {
        m_zmqToTcpThread->join();
    }
    if (m_tcpToZmqThread && m_tcpToZmqThread->joinable())
    {
        m_tcpToZmqThread->join();
    }

    // Clean up resources
    try
    {
        if (m_tcpServer)
        {
            m_tcpServer->Close();
            m_tcpServer.reset();
        }

        if (m_zmqRecvSocket)
        {
            m_zmqRecvSocket->close();
            m_zmqRecvSocket.reset();
        }

        if (m_zmqSendSocket)
        {
            m_zmqSendSocket->close();
            m_zmqSendSocket.reset();
        }

        if (m_zmqContext)
        {
            m_zmqContext->close();
            m_zmqContext.reset();
        }
    }
    catch (const zmq::error_t& e)
    {
        std::cerr << "Error during cleanup: " << e.what() << std::endl;
    }

    m_zmqToTcpThread.reset();
    m_tcpToZmqThread.reset();

    std::cout << "Bridge stopped" << std::endl;
    return 0;
}

std::string cISZmqTcpBridge::GetStatus() const
{
    if (m_isRunning)
    {
        return "Running - ZMQ Recv: " + m_zmqRecvEndpoint + ", ZMQ Send: " + m_zmqSendEndpoint + ", TCP Port: " + std::to_string(m_tcpPort);
    }
    return "Stopped";
}

void cISZmqTcpBridge::ZmqToTcpForwardingThread()
{
    while (m_isRunning)
    {
        try
        {
            // Receive from ZMQ (non-blocking with timeout)
            zmq::message_t message;
            zmq::recv_result_t result = m_zmqRecvSocket->recv(message, zmq::recv_flags::none);

            if (result.has_value() && message.size() > 0)
            {
                // Forward to all connected TCP clients
                if (m_tcpServer && m_tcpServer->IsOpen())
                {
                    m_tcpServer->Write(message.data(), static_cast<int>(message.size()));
                }
            }
        }
        catch (const zmq::error_t& e)
        {
            if (e.num() != EAGAIN && e.num() != EINTR)
            {
                std::cerr << "ZMQ receive error: " << e.what() << std::endl;
            }
            // Continue even on error (timeout is expected)
        }

        // Small sleep to prevent busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void cISZmqTcpBridge::TcpToZmqForwardingThread()
{
    while (m_isRunning)
    {
        try
        {
            // Update TCP server to accept new connections and handle disconnections
            if (m_tcpServer && m_tcpServer->IsOpen())
            {
                m_tcpServer->Update();
            }

            // Note: TCP → ZMQ forwarding would require implementing a custom
            // TCP server delegate to handle incoming data. For now, this
            // implementation focuses on ZMQ → TCP forwarding which is the
            // primary use case (IMU data streaming).
            //
            // If bidirectional communication is needed, the TCP server would
            // need a delegate that captures data and forwards it through
            // m_zmqSendSocket.

            // Small sleep to prevent busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        catch (const std::exception& e)
        {
            std::cerr << "TCP forwarding error: " << e.what() << std::endl;
        }
    }
}
