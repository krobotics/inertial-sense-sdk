CMAKE_MINIMUM_REQUIRED(VERSION 3.10.0)

project(InertialSenseSDKWithZMQ)

# Add the InertialSense SDK submodule
add_subdirectory(inertialsense)

# Find ZMQ library for the bridge
find_library(ZMQ_LIB zmq)
if(NOT ZMQ_LIB)
    message(WARNING "ZMQ library not found. ZMQ-TCP bridge will not be available.")
else()
    message(STATUS "ZMQ library found: ${ZMQ_LIB}")
    # Build ZMQ-TCP bridge if ZMQ is available
    option(BUILD_ZMQ_TCP_BRIDGE "Build the ZMQ-to-TCP bridge library and executable" ON)
    if(BUILD_ZMQ_TCP_BRIDGE)
        add_subdirectory(zmq-tcp-bridge)
    endif()
endif()
