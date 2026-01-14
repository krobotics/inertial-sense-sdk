CMAKE_MINIMUM_REQUIRED(VERSION 3.10.0)

project(ISAsciiExample)

set(IS_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
include(${IS_SDK_DIR}/include_is_sdk_find_library.cmake)

# Include InertialSenseSDK header files
include_directories(
    ${IS_SDK_DIR}/src
    ${IS_SDK_DIR}/src/libusb/libusb
)

# Link the InertialSenseSDK static library 
link_directories(${IS_SDK_DIR})

# Define the executable
add_executable(${PROJECT_NAME} ISAsciiExample.cpp)

# Find ZMQ library
find_library(ZMQ_LIB zmq)

# Link InertialSenseSDK static library to the executable
target_link_libraries(${PROJECT_NAME} InertialSenseSDK)

# Link ZMQ if available
if(ZMQ_LIB)
    target_link_libraries(${PROJECT_NAME} ${ZMQ_LIB})
endif()
