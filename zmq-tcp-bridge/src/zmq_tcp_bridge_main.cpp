/*
MIT LICENSE

Copyright (c) 2014-2025 Inertial Sense, Inc. - http://inertialsense.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ISZmqTcpBridge.h"
#include <iostream>
#include <csignal>
#include <string>
#include <cstring>

static cISZmqTcpBridge* g_bridge = nullptr;

void signalHandler(int signum)
{
    std::cout << "\nInterrupt signal (" << signum << ") received." << std::endl;
    if (g_bridge)
    {
        g_bridge->Stop();
    }
    exit(signum);
}

void printUsage(const char* progName)
{
    std::cout << "Usage: " << progName << " [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "ZMQ-to-TCP Bridge for InertialSense SDK" << std::endl;
    std::cout << "Forwards data between ZMQ sockets and TCP connections" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --zmq-recv <endpoint>    ZMQ endpoint to receive from (default: tcp://127.0.0.1:7115)" << std::endl;
    std::cout << "  --zmq-send <endpoint>    ZMQ endpoint to send to (default: tcp://127.0.0.1:7116)" << std::endl;
    std::cout << "  --tcp-port <port>        TCP port for SDK clients (default: 8000)" << std::endl;
    std::cout << "  -h, --help               Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << progName << std::endl;
    std::cout << "  " << progName << " --tcp-port 9000" << std::endl;
    std::cout << "  " << progName << " --zmq-recv tcp://127.0.0.1:7115 --zmq-send tcp://127.0.0.1:7116 --tcp-port 8000" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    // Default configuration
    std::string zmqRecvEndpoint = "tcp://127.0.0.1:7115";
    std::string zmqSendEndpoint = "tcp://127.0.0.1:7116";
    int tcpPort = 8000;

    // Parse command line arguments
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--zmq-recv") == 0 && i + 1 < argc)
        {
            zmqRecvEndpoint = argv[++i];
        }
        else if (strcmp(argv[i], "--zmq-send") == 0 && i + 1 < argc)
        {
            zmqSendEndpoint = argv[++i];
        }
        else if (strcmp(argv[i], "--tcp-port") == 0 && i + 1 < argc)
        {
            tcpPort = std::stoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            printUsage(argv[0]);
            return 0;
        }
        else
        {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // Register signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Create and start bridge
    cISZmqTcpBridge bridge;
    g_bridge = &bridge;

    std::cout << "Starting ZMQ-to-TCP Bridge..." << std::endl;
    
    if (bridge.Start(zmqRecvEndpoint, zmqSendEndpoint, tcpPort) != 0)
    {
        std::cerr << "Failed to start bridge" << std::endl;
        return 1;
    }

    std::cout << "Bridge is running. Press Ctrl+C to stop." << std::endl;
    std::cout << "SDK clients can connect to TCP port " << tcpPort << std::endl;
    std::cout << "Connection string example: TCP:IS:127.0.0.1:" << tcpPort << std::endl;

    // Keep running until interrupted
    while (bridge.IsRunning())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    g_bridge = nullptr;
    return 0;
}
