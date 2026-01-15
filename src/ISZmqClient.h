/*
Copyright (c) 2025 RED 6
*/

#ifndef __ISZMQCLIENT__H__
#define __ISZMQCLIENT__H__

#include <string>
#include <inttypes.h>
#include <memory>
#include <vector>

#include "ISStream.h"
#include "ISComm.h"

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

    // ISB packet validation
    is_comm_instance_t m_comm;
    std::vector<uint8_t> m_commBuffer;
};

#endif // __ISZMQCLIENT__H__
