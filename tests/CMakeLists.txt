CMAKE_MINIMUM_REQUIRED(VERSION 3.10.0)
if(WIN32)
set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")	# Must go before calling project()
endif()

project(IS-SDK_unit-tests)

add_definitions(-DSDK_UNIT_TEST)

set(IS_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/..")
include(${IS_SDK_DIR}/include_is_sdk_find_library.cmake)

# Include InertialSenseSDK header files
include_directories(
    ${IS_SDK_DIR}/src
    ${IS_SDK_DIR}/src/protocol
    ${IS_SDK_DIR}/src/libusb/libusb
    ${IS_SDK_DIR}/src/util
    ${IS_SDK_DIR}/tests/runtime
)

# Add system libusb include directory for Linux
if(UNIX)
    find_path(LIBUSB_INCLUDE_DIR libusb.h PATH_SUFFIXES libusb-1.0)
    if(LIBUSB_INCLUDE_DIR)
        include_directories(${LIBUSB_INCLUDE_DIR})
    endif()
endif()

find_package(GTest REQUIRED)

# Find ZMQ library
find_library(ZMQ_LIB zmq)
if(NOT ZMQ_LIB)
    # Try to use the library directly if find_library doesn't work
    set(ZMQ_LIB "zmq")
endif()

# Link the InertialSenseSDK static library 
link_directories(${IS_SDK_DIR})

file(GLOB TESTS_SOURCES "${CMAKE_CURRENT_LIST_DIR}/test_*.cpp")
list(FILTER TESTS_SOURCES EXCLUDE REGEX "test_com_manager_2.cpp")
if(WIN32)   # These do not run in Windows
    list(FILTER TESTS_SOURCES EXCLUDE REGEX "test_time_conversion.cpp")
    # yamlcpp needs to know it's a static lib
    add_definitions(-DYAML_CPP_STATIC_DEFINE)
endif()

# Define the executable
add_executable(${PROJECT_NAME} 
    ${TESTS_SOURCES}
    runtime/device_runtime_tests.cpp

    # test_ISLogger.cpp
    # test_data_utils.cpp

    # test_protocol_nmea.cpp
    # test_time_conversion.cpp    
    # test_runtime_tests.cpp
    # test_data_utils.cpp
    # test_ISDataMappings.cpp
)

if(WIN32)   # Windows
    set(PROJECT_PLATFORM_LIBS GTest::GTest GTest::Main)
    # Link ZMQ if available on Windows
    if(ZMQ_LIB)
        list(APPEND PROJECT_PLATFORM_LIBS ${ZMQ_LIB})
    endif()
else()      # Linux
    set(PROJECT_PLATFORM_LIBS gtest_main ${GTEST_LIBRARIES} udev pthread m usb-1.0)
endif()

# Link InertialSenseSDK static library to the executable
# Link platform libraries and ZMQ after InertialSenseSDK to resolve symbols
target_link_libraries(${PROJECT_NAME} InertialSenseSDK ${PROJECT_PLATFORM_LIBS})
if(ZMQ_LIB)
    target_link_libraries(${PROJECT_NAME} ${ZMQ_LIB})
endif()
