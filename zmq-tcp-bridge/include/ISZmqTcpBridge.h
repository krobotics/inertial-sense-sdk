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
#include <mutex>
#include "ISTcpServer.h"

// Forward declarations to avoid including headers
namespace zmq {
    class context_t;
    class socket_t;
}

/**
 * ZMQ-to-TCP Bridge
 * 
 * This class creates a bidirectional bridge between ZMQ sockets and TCP connections.
 * It allows the original InertialSense SDK to communicate via TCP without needing ZMQ support.
 * 
 * Architecture:
 * - ZMQ Publisher (external) ←→ ZMQ SUB/PUB sockets ←→ Bridge ←→ TCP Server ←→ SDK Client
 * 
 * The bridge implements full bidirectional communication:
 * - ZMQ → TCP: Data from ZMQ publishers is forwarded to connected TCP clients
 * - TCP → ZMQ: Data from TCP clients is forwarded to ZMQ subscribers
 * 
 * Usage:
 * 1. Start the bridge with ZMQ endpoints and TCP port
 * 2. Connect InertialSense SDK client to TCP port using normal TCP connection string
 * 3. Data flows bidirectionally between ZMQ and TCP transparently
 */
class cISZmqTcpBridge : public iISTcpServerDelegate
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

protected:
    /**
     * Delegate method called when TCP client data is received
     * Forwards the data to ZMQ send socket for TCP → ZMQ communication
     * @param server the TCP server receiving data
     * @param socket the client socket
     * @param data the data received
     * @param dataLength the number of bytes received
     */
    void OnClientDataReceived(cISTcpServer* server, is_socket_t socket, uint8_t* data, int dataLength) override;

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
    std::mutex m_zmqSendMutex;  // Protects access to m_zmqSendSocket
    
    // Configuration
    std::string m_zmqRecvEndpoint;
    std::string m_zmqSendEndpoint;
    int m_tcpPort;
};

#endif // __ISZMQTCPBRIDGE__H__
