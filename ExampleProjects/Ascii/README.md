# SDK: ASCII Communications Example Project

This [ISAsciiExample](https://github.com/inertialsense/inertial-sense-sdk/tree/release/ExampleProjects/Ascii) project demonstrates NMEA communications with the <a href="https://inertialsense.com">InertialSense</a> products (uINS, uAHRS, and uIMU) using the Inertial Sense SDK. The example supports both **serial port** and **ZMQ** connections. See the [ASCII protocol](../protocol_ascii) section for details on the ASCII packet structures. 

## Files

#### Project Files

* [ISAsciiExample.cpp](https://github.com/inertialsense/inertial-sense-sdk/tree/release/ExampleProjects/Ascii/ISAsciiExample.cpp)

#### SDK Files

* [ISClient.cpp](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/ISClient.cpp)
* [ISClient.h](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/ISClient.h)
* [ISStream.h](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/ISStream.h)
* [ISZmqClient.cpp](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/ISZmqClient.cpp)
* [ISZmqClient.h](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/ISZmqClient.h)
* [ISSerialPort.cpp](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/ISSerialPort.cpp)
* [ISSerialPort.h](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/ISSerialPort.h)
* [data_sets.c](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/data_sets.c)
* [data_sets.h](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/data_sets.h)
* [ISComm.c](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/ISComm.c)
* [ISComm.h](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/ISComm.h)
* [ISConstants.h](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/ISConstants.h)
* [serialPort.c](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/serialPort.c)
* [serialPort.h](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/serialPort.h)
* [serialPortPlatform.c](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/serialPortPlatform.c)
* [serialPortPlatform.h](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src/serialPortPlatform.h)

## Implementation

### Step 1: Add Includes

```C++
// Change these include paths to the correct paths for your project
#include "../../src/ISClient.h"
#include "../../src/ISStream.h"
```

### Step 2: Open connection (Serial or ZMQ)

The example now supports both serial port and ZMQ connections. The connection type is automatically detected based on the connection string format.

```C++
std::string connectionString = argv[1];
cISStream* stream = nullptr;

// Check if the connection string is a ZMQ connection (starts with "ZMQ:")
if (connectionString.find("ZMQ:") == 0)
{
    // Use cISClient to open ZMQ connection
    stream = cISClient::OpenConnectionToServer(connectionString);
    if (stream == nullptr)
    {
        printf("Failed to open ZMQ connection: %s\n", connectionString.c_str());
        return -2;
    }
    printf("Connected via ZMQ: %s\n", stream->ConnectionInfo().c_str());
}
else
{
    // Use cISClient to open serial connection
    // Format: SERIAL:IS:port:baudrate
    std::string serialConnectionString = "SERIAL:IS:" + connectionString + ":921600";
    stream = cISClient::OpenConnectionToServer(serialConnectionString);
    if (stream == nullptr)
    {
        printf("Failed to open serial port: %s\n", connectionString.c_str());
        return -2;
    }
    printf("Connected via Serial: %s\n", stream->ConnectionInfo().c_str());
}
```

**Connection String Formats:**
- **Serial Port:** `/dev/ttyACM0` or `COM3` (automatically converted to `SERIAL:IS:port:921600`)
- **ZMQ:** `ZMQ:IS:send_port:recv_port` (e.g., `ZMQ:IS:7116:7115`)

### Step 3: Stop prior message broadcasting

```C++
// Stop all broadcasts on the device on all ports. We don't want binary messages coming through while we are doing ASCII
if (!streamWriteAscii(stream, "STPB", 4))
{
    printf("Failed to encode stop broadcasts message\n");
    stream->Close();
    delete stream;
    return -3;
}
```

### Step 4: Enable ASCII message broadcasting

```C++
	// ASCII protocol is based on NMEA protocol https://en.wikipedia.org/wiki/NMEA_0183
	// turn on the INS message at a period of 100 milliseconds (10 hz)
	// streamWriteAscii takes care of the leading $ character, checksum and ending \r\n newline
	// ASCE message enables ASCII broadcasts
	// ASCE fields: 1:options, ID0, Period0, ID1, Period1, ........ ID19, Period19
	// IDs:
	// NMEA_MSG_ID_PIMU      = 0,
    // NMEA_MSG_ID_PPIMU     = 1,
    // NMEA_MSG_ID_PRIMU     = 2,
    // NMEA_MSG_ID_PINS1     = 3,
    // NMEA_MSG_ID_PINS2     = 4,
    // NMEA_MSG_ID_PGPSP     = 5,
    // NMEA_MSG_ID_GNGGA     = 6,
    // NMEA_MSG_ID_GNGLL     = 7,
    // NMEA_MSG_ID_GNGSA     = 8,
    // NMEA_MSG_ID_GNRMC     = 9,
    // NMEA_MSG_ID_GNZDA     = 10,
    // NMEA_MSG_ID_PASHR     = 11, 
    // NMEA_MSG_ID_PSTRB     = 12,
    // NMEA_MSG_ID_INFO      = 13,
    // NMEA_MSG_ID_GNGSV     = 14,
    // NMEA_MSG_ID_GNVTG     = 15,
    // NMEA_MSG_ID_INTEL     = 16,

	// options can be 0 for current serial port, 1 for serial 0, 2 for serial 1 or 3 for both serial ports
	// Instead of a 0 for a message, it can be left blank (,,) to not modify the period for that message
	// please see the user manual for additional updates and notes

    // Get PINS1 @ 5Hz on the connected serial port, leave all other broadcasts the same, and save persistent messages.
	const char* asciiMessage = "ASCE,0,3,1";

    // Get PINS1 @ 1Hz and PGPSP @ 1Hz on the connected serial port, leave all other broadcasts the same
	// const char* asciiMessage = "ASCE,0,5,5";

	// Get PIMU @ 50Hz, GGA @ 5Hz, serial0 and serial1 ports, set all other periods to 0
    //  const char* asciiMessage = "ASCE,3,6,1";

	// Stop all messages / broadcasts
	// const char* asciiMessage = "STPB";
																				
	if (!streamWriteAscii(stream, asciiMessage, (int)strnlen(asciiMessage, 128)))
	{
		printf("Failed to encode ASCII get INS message\n");
		stream->Close();
		delete stream;
		return -4;
	}
```


### Step 5: Handle received data 

```C++
	// Handle received data
	unsigned char* asciiData;
	unsigned char asciiLine[512];

	printf("Listening for ASCII messages... (Press Ctrl+C to exit)\n");

	// you can set running to false with some other piece of code to break out of the loop and end the program
	while (running)
	{
		if (streamReadAscii(stream, asciiLine, sizeof(asciiLine), &asciiData) > 0)
		{
			printf("%s", asciiData);
		}
	}

	// Clean up
	stream->Close();
	delete stream;
```

## Compile & Run (Linux/Mac)

1. Install necessary dependencies
   ``` bash
   # For Debian/Ubuntu linux, install libusb-1.0-0-dev from packages
   sudo apt update && sudo apt install libusb-1.0-0-dev
   # For MacOS, install libusb using brew
   brew install libusb
   ```
2. Create build directory
   ``` bash
   cd inertial-sense-sdk/ExampleProjects/Ascii
   mkdir build
   ```
3. Run cmake from within build directory
   ``` bash
   cd build
   cmake ..
   ```
4. Compile using make
    ``` bash
    make
    ```
5. If necessary, add current user to the "dialout" group to read and write to the USB serial communication ports.  In some cases the Modem Manager must be disabled to prevent interference with serial communication. 
   ```bash
   sudo usermod -a -G dialout $USER
   sudo usermod -a -G plugdev $USER
   sudo systemctl disable ModemManager.service && sudo systemctl stop ModemManager.service
   (reboot computer)
   ```
6. Run executable with serial port
   ``` bash
   ./bin/ISAsciiExample /dev/ttyUSB0
   ```
7. Or run with ZMQ connection
   ``` bash
   ./bin/ISAsciiExample ZMQ:IS:7116:7115
   ```
## Compile & Run (Windows MS Visual Studio)

1. Open Visual Studio solution file (inertial-sense-sdk\ExampleProjects\Ascii\VS_project\ISAsciiExample.sln)
2. Build (F7)
3. Run executable with serial port
   ``` bash
   C:\inertial-sense-sdk\ExampleProjects\Ascii\VS_project\Release\ISAsciiExample.exe COM3
   ```
4. Or run with ZMQ connection
   ``` bash
   C:\inertial-sense-sdk\ExampleProjects\Ascii\VS_project\Release\ISAsciiExample.exe ZMQ:IS:7116:7115
   ```

## Summary

That covers all the basic functionality you need to set up and talk to <a href="https://inertialsense.com">InertialSense</a> products.  If this doesn't cover everything you need, feel free to reach out to us on the <a href="https://github.com/inertialsense/inertial-sense-sdk">inertial-sense-sdk</a> GitHub repository, and we will be happy to help.
