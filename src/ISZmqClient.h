/*
MIT LICENSE

Copyright (c) 2014-2025 Inertial Sense, Inc. - http://inertialsense.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef __ISZMQCLIENT__H__
#define __ISZMQCLIENT__H__

#include <string>
#include <inttypes.h>
#include <memory>

#include "ISStream.h"

// ZMQ endpoint macros
#define ZMQ_STRINGIFY_PORT(x) #x
#define ZMQ_STRINGIFY(x) ZMQ_STRINGIFY_PORT(x)
#define ZMQ_IPC_ENDPOINT(port) ("tcp://127.0.0.1:" ZMQ_STRINGIFY(port))

#define ENDPOINT_HEADSET_1_IMU_TO_CLIENT           ZMQ_IPC_ENDPOINT(7115)
#define ENDPOINT_HEADSET_1_CLIENT_TO_IMU           ZMQ_IPC_ENDPOINT(7116)
#define ENDPOINT_HEADSET_2_IMU_TO_CLIENT           ZMQ_IPC_ENDPOINT(7135)
#define ENDPOINT_HEADSET_2_CLIENT_TO_IMU           ZMQ_IPC_ENDPOINT(7136)

// Forward declaration to avoid including zmq.hpp in header
namespace zmq {
    class context_t;
    class socket_t;
}

class cISZmqClient : public cISStream
{
public:
    /**
    * Constructor
    */
    cISZmqClient();

    /**
    * Destructor
    */
    virtual ~cISZmqClient();

    /**
    * Opens ZMQ sockets for bidirectional communication
    * @param sendEndpoint the endpoint to send data to (client_to_imu)
    * @param recvEndpoint the endpoint to receive data from (imu_to_client)
    * @return 0 if success, otherwise an error code
    */
    int Open(const std::string& sendEndpoint, const std::string& recvEndpoint);

    /**
    * Close the client
    * @return 0 if success, otherwise an error code
    */
    int Close() OVERRIDE;

    /**
    * Read data from the client
    * @param data the buffer to read data into
    * @param dataLength the number of bytes available in data
    * @return the number of bytes read or less than 0 if error
    */
    int Read(void* data, int dataLength) OVERRIDE;

    /**
    * Write data to the client
    * @param data the data to write
    * @param dataLength the number of bytes to write
    * @return the number of bytes written or less than 0 if error
    */
    int Write(const void* data, int dataLength) OVERRIDE;

    /**
    * Get whether the connection is open
    * @return true if connection open, false if not
    */
    bool IsOpen() { return m_isOpen; }

    /**
    * Gets information about the current connection
    * @return connection info
    */
    std::string ConnectionInfo() OVERRIDE;

private:
    cISZmqClient(const cISZmqClient& copy); // Disable copy constructor

    std::unique_ptr<zmq::context_t> m_context;
    std::unique_ptr<zmq::socket_t> m_sendSocket;    // client_to_imu
    std::unique_ptr<zmq::socket_t> m_recvSocket;    // imu_to_client
    std::string m_sendEndpoint;
    std::string m_recvEndpoint;
    bool m_isOpen;
};

#endif // __ISZMQCLIENT__H__
