CMAKE_MINIMUM_REQUIRED(VERSION 3.10.0)

project(ZMQTCPBridge)

# Find ZMQ library (required for the bridge)
find_library(ZMQ_LIB zmq)
if(NOT ZMQ_LIB)
    message(FATAL_ERROR "ZMQ library is required for the ZMQ-TCP bridge. Please install libzmq.")
endif()

# Bridge library sources
set(BRIDGE_SOURCES
    src/ISZmqTcpBridge.cpp
)

set(BRIDGE_HEADERS
    include/ISZmqTcpBridge.h
)

# Create shared library
add_library(${PROJECT_NAME} SHARED ${BRIDGE_SOURCES} ${BRIDGE_HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    include
    ../src
    ../external
)

# Link against InertialSenseSDK and ZMQ
target_link_libraries(${PROJECT_NAME}
    InertialSenseSDK
    ${ZMQ_LIB}
)

# Set C++ standard
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)

# Build the standalone bridge executable
add_executable(zmq_tcp_bridge src/zmq_tcp_bridge_main.cpp)

target_include_directories(zmq_tcp_bridge PRIVATE
    include
    ../src
    ../external
)

# Link libraries for the executable
target_link_libraries(zmq_tcp_bridge
    ${PROJECT_NAME}
    InertialSenseSDK
    ${ZMQ_LIB}
)

# Link pthread only on Unix-like platforms (not needed on Windows)
if(UNIX)
    target_link_libraries(zmq_tcp_bridge pthread)
endif()

set_property(TARGET zmq_tcp_bridge PROPERTY CXX_STANDARD 20)

# Install targets
install(TARGETS ${PROJECT_NAME} zmq_tcp_bridge
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${BRIDGE_HEADERS} DESTINATION include)
