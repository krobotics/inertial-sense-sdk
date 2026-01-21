/*
MIT LICENSE

Copyright (c) 2014-2025 Inertial Sense, Inc. - http://inertialsense.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef __ISZMQTCPBRIDGE__H__
#define __ISZMQTCPBRIDGE__H__

#include <string>
#include <memory>
#include <thread>
#include <atomic>

// Forward declarations to avoid including headers
namespace zmq {
    class context_t;
    class socket_t;
}

class cISTcpServer;

/**
 * ZMQ-to-TCP Bridge
 * 
 * This class creates a unidirectional bridge from ZMQ sockets to TCP connections.
 * It allows the original InertialSense SDK to receive data via TCP without needing ZMQ support.
 * 
 * Architecture:
 * - ZMQ Publisher (external) → ZMQ SUB socket → Bridge → TCP Server → SDK Client
 * 
 * Note: Currently implements ZMQ → TCP forwarding (primary use case for IMU data streaming).
 * TCP → ZMQ forwarding can be added if bidirectional communication is needed.
 * 
 * Usage:
 * 1. Start the bridge with ZMQ endpoints and TCP port
 * 2. Connect InertialSense SDK client to TCP port using normal TCP connection string
 * 3. Data flows from ZMQ to TCP transparently
 */
class cISZmqTcpBridge
{
public:
    /**
     * Constructor
     */
    cISZmqTcpBridge();

    /**
     * Destructor
     */
    virtual ~cISZmqTcpBridge();

    /**
     * Start the bridge
     * @param zmqRecvEndpoint ZMQ endpoint to receive data from (e.g., "tcp://127.0.0.1:7115")
     * @param zmqSendEndpoint ZMQ endpoint to send data to (e.g., "tcp://127.0.0.1:7116")
     * @param tcpPort TCP port for SDK clients to connect to
     * @return 0 if success, otherwise an error code
     */
    int Start(const std::string& zmqRecvEndpoint, const std::string& zmqSendEndpoint, int tcpPort);

    /**
     * Stop the bridge
     * @return 0 if success, otherwise an error code
     */
    int Stop();

    /**
     * Check if bridge is running
     * @return true if running, false otherwise
     */
    bool IsRunning() const { return m_isRunning; }

    /**
     * Get bridge status information
     * @return status string
     */
    std::string GetStatus() const;

private:
    cISZmqTcpBridge(const cISZmqTcpBridge&) = delete;
    cISZmqTcpBridge& operator=(const cISZmqTcpBridge&) = delete;

    /**
     * Thread function for forwarding ZMQ → TCP
     */
    void ZmqToTcpForwardingThread();

    /**
     * Thread function for forwarding TCP → ZMQ
     */
    void TcpToZmqForwardingThread();

    // ZMQ context and sockets
    std::unique_ptr<zmq::context_t> m_zmqContext;
    std::unique_ptr<zmq::socket_t> m_zmqRecvSocket;  // SUB socket for receiving from ZMQ
    std::unique_ptr<zmq::socket_t> m_zmqSendSocket;  // PUB socket for sending to ZMQ
    
    // TCP server
    std::unique_ptr<cISTcpServer> m_tcpServer;
    
    // Threading
    std::unique_ptr<std::thread> m_zmqToTcpThread;
    std::unique_ptr<std::thread> m_tcpToZmqThread;
    std::atomic<bool> m_isRunning;
    
    // Configuration
    std::string m_zmqRecvEndpoint;
    std::string m_zmqSendEndpoint;
    int m_tcpPort;
};

#endif // __ISZMQTCPBRIDGE__H__
