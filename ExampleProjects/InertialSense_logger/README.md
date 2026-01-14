# SDK: Data Logging Example Project

This [ISLoggerExample](https://github.com/inertialsense/inertial-sense-sdk/tree/main/ExampleProjects/ISLogger) project demonstrates how to use the **InertialSense** c++ class to enable data streaming and log data to file from [InertialSense](https://inertialsense.com) products (IMX and GPX).

## Files

#### Project Files

* [ISLoggerExample.cpp](https://github.com/inertialsense/inertial-sense-sdk/tree/main/ExampleProjects/ISLogger/ISLoggerExample.cpp)

#### SDK Files

* [SDK](https://github.com/inertialsense/inertial-sense-sdk/tree/main/src)


## Implementation

### Step 1: Add Includes

```C++
#include "InertialSense.h"
```

### Step 2: Instantiate InertialSense class and Open Connection

The logger supports both serial port and ZMQ connections:

```C++
	// InertialSense class wraps communications and logging in a convenient, easy to use class
	InertialSense inertialSense(dataCallback);
	
	// Check if the connection string is a ZMQ connection (starts with "ZMQ:")
	std::string connectionString = argv[1];
	bool isZmq = (connectionString.find("ZMQ:") == 0);
	
	bool connected = false;
	if (isZmq)
	{
		// Use OpenConnectionToServer for ZMQ connections
		connected = inertialSense.OpenConnectionToServer(connectionString);
	}
	else
	{
		// Use Open for serial port connections
		connected = inertialSense.Open(connectionString);
	}
	
	if (!connected)
	{
		std::cout << "Failed to open connection: " << connectionString << std::endl;
		return -1;
	}
```

### Step 3: Enable data logger

```C++
	// get log type from command line
	cISLogger::eLogType logType = (argc < 3 ? cISLogger::eLogType::LOGTYPE_DAT : cISLogger::ParseLogType(argv[2]));
	inertialSense.EnableLogger(true, "", logType);
```

### Step 4: Enable data broadcasting

```C++
	// broadcast the standard set of post processing messages (ins, imu, etc.)
	// Note: RMC_PRESET_INS includes DID_INS_2 and DID_GPS1_POS data
	inertialSense.BroadcastBinaryDataRmcPreset();

	// instead of the rmc preset (real-time message controller) you can request individual messages...
	// inertialSense.BroadcastBinaryData(DID_INS_2, 10); // ins_2 every 40 milliseconds (25 hz) - period multiple 10 x 4ms source period
	// inertialSense.BroadcastBinaryData(DID_IMU, 10); // imu every 40 milliseconds (25 hz) - period multiple 10 x 4ms source period
```

By default, data logs will be stored in the "IS_logs" directory in the current directory.

``` bash
build/IS_logs/LOG_SN30664_20180323_112822_0001.dat
```

## Connection Types

This example supports two types of connections:

### Serial Port Connection
Direct connection to InertialSense device via USB/serial port:
- Linux: `/dev/ttyUSB0`, `/dev/ttyACM0`
- Windows: `COM3`, `COM4`

### ZMQ Connection
Network connection using ZeroMQ for communication with a device or simulator:
- Format: `ZMQ:IS:send_port:recv_port`
- Example: `ZMQ:IS:7116:7115`

For ZMQ connections, ensure the ZMQ library is installed:
- **Linux:** `sudo apt-get install libzmq3-dev`
- **Windows:** Install ZMQ via vcpkg or from https://zeromq.org/

See [ZMQ Interface Documentation](../../docs/ZMQ_Interface.md) for more details.

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
   cd inertial-sense-sdk/ExampleProjects/Logger
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
6. Run executable
   
   **Serial Port Connection:**
   ``` bash
   ./build/ISLoggerExample /dev/ttyUSB0
   ```
   
   **ZMQ Connection:**
   ``` bash
   # Format: ZMQ:IS:send_port:recv_port
   ./build/ISLoggerExample ZMQ:IS:7116:7115
   ```
   
   **With Log Type:**
   ``` bash
   ./build/ISLoggerExample /dev/ttyUSB0 raw
   ./build/ISLoggerExample ZMQ:IS:7116:7115 dat
   ```
## Compile & Run (Windows MS Visual Studio)

1. **Open Visual Studio:** Start Microsoft Visual Stdio.
2. **Open the CMake Project:** 

   - Go to **File** > **Open** > **CMake...**.
   - Navigate to the folder containing your CMake project (where the **CMakeLists.txt** file is located).
   - Select the **CMakeLists.txt** file and click **Open**.

3. **Build the Project:** (F7)

4. **Run the Executable:** 

   **Serial Port Connection:**
   ``` bash
   C:\inertial-sense-sdk\ExampleProjects\ISLogger\VS_project\Release\ISLoggerExample.exe COM3
   ```
   
   **ZMQ Connection:**
   ``` bash
   C:\inertial-sense-sdk\ExampleProjects\ISLogger\VS_project\Release\ISLoggerExample.exe ZMQ:IS:7116:7115
   ```

## Summary

That covers all the basic functionality you need to set up and talk to <a href="https://inertialsense.com">InertialSense</a> products.  If this doesn't cover everything you need, feel free to reach out to us on the <a href="https://github.com/inertialsense/inertial-sense-sdk">inertial-sense-sdk</a> GitHub repository, and we will be happy to help.
