/*
MIT LICENSE

Copyright (c) 2014-2025 Inertial Sense, Inc. - http://inertialsense.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files(the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <string.h>
#include <string>

// STEP 1: Add Includes
// Change these include paths to the correct paths for your project
#include "../../src/ISClient.h"
#include "../../src/ISStream.h"

static int running = 1;

// Helper function to write ASCII message to stream with NMEA checksum
static int streamWriteAscii(cISStream* stream, const char* buffer, int bufferLength)
{
    if (stream == nullptr || buffer == nullptr || bufferLength < 2)
    {
        return 0;
    }

    int checkSum = 0;
    const unsigned char* ptr = (const unsigned char*)buffer;
    int count = 0;

    // Add leading '$' if not present
    if (*buffer != '$')
    {
        count += stream->Write((const unsigned char*)"$", 1);
    }
    else
    {
        // Skip the '$' for checksum calculation
        ptr++;
        bufferLength--;
    }

    const unsigned char* ptrEnd = ptr + bufferLength;
    unsigned char buf[16];

    // Write the message content (without leading $)
    count += stream->Write(ptr, bufferLength);

    // Calculate checksum
    while (ptr < ptrEnd)
    {
        checkSum ^= *ptr++;
    }

    // Write checksum and line ending
    snprintf((char*)buf, sizeof(buf), "*%.2x\r\n", checkSum);
    count += stream->Write(buf, 5);

    return count;
}

// Helper function to read ASCII line from stream
static int streamReadLine(cISStream* stream, unsigned char* buffer, int bufferLength)
{
    if (stream == nullptr || buffer == nullptr || bufferLength < 1)
    {
        return 0;
    }

    int count = 0;
    unsigned char c;

    while (count < bufferLength - 1)
    {
        int bytesRead = stream->Read(&c, 1);
        if (bytesRead <= 0)
        {
            break;
        }

        buffer[count++] = c;

        // Check for end of line
        if (c == '\n')
        {
            break;
        }
    }

    buffer[count] = '\0';
    return count;
}

// Helper function to read and validate ASCII message from stream
static int streamReadAscii(cISStream* stream, unsigned char* buffer, int bufferLength, unsigned char** asciiData)
{
    int count = streamReadLine(stream, buffer, bufferLength);
    if (count < 8)
    {
        return -1;
    }

    unsigned char* ptr = buffer;
    unsigned char* ptrEnd = buffer + count;
    
    // Find start of NMEA message ($)
    while (*ptr != '$' && ptr < ptrEnd)
    {
        ptr++;
    }

    // if at least 8 chars available
    if (ptrEnd - ptr > 7)
    {
        if (asciiData != nullptr)
        {
            *asciiData = ptr;
        }
        
        int checksum = 0;
        int existingChecksum;

        // calculate checksum, skipping leading $ and trailing *XX\r\n (5 chars)
        unsigned char* ptrEndNoChecksum = ptrEnd - 5;
        while (++ptr < ptrEndNoChecksum)
        {
            checksum ^= *ptr;
        }

        if (*ptr == '*')
        {
            // read checksum from buffer, skipping the * char
            existingChecksum = strtol((char*)++ptr, NULL, 16);
            if (existingChecksum == checksum)
            {
                return count;
            }
        }
    }

    return -1;
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("Usage: $ ./ISAsciiExample <connection>\n");
		printf("  connection: Serial port (e.g., /dev/ttyACM0, COM3) or ZMQ connection string (ZMQ:IS:send_port:recv_port)\n");
		printf("\nExamples:\n");
		printf("  $ ./ISAsciiExample /dev/ttyACM0\n");
		printf("  $ ./ISAsciiExample ZMQ:IS:7116:7115\n");
		// In Visual Studio IDE, this can be done through "Project Properties -> Debugging -> Command Arguments: COM3" 
		return -1;
	}

	// STEP 2: Open connection (Serial or ZMQ)
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

	// STEP 3: Stop prior message broadcasting
	// Stop all broadcasts on the device on all ports. We don't want binary messages coming through while we are doing ASCII
	if (!streamWriteAscii(stream, "STPB", 4))
	{
		printf("Failed to encode stop broadcasts message\n");
		stream->Close();
		delete stream;
		return -3;
	}
#if 0
    // Query device version information
	if (!streamWriteAscii(stream, "INFO", 4))
	{
		printf("Failed to encode info message\n");
		stream->Close();
		delete stream;
		return -3;
	}
#endif
    // STEP 4: Enable message broadcasting

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

    
    if (!streamWriteAscii(stream, asciiMessage, (int)strnlen(asciiMessage, 128)))
	{
		printf("Failed to encode ASCII get INS message\n");
		stream->Close();
		delete stream;
		return -4;
	}


#if 0
    // STEP 5: (optional) Save Persistent Messages.  This remembers the current communications and automatically streams data following reboot.
    if (!streamWriteAscii(stream, "PERS", 4))
    {
        printf("Failed to encode ASCII save persistent message\n");
		stream->Close();
		delete stream;
        return -4;
    }
#endif


	// STEP 6: Handle received data
	unsigned char* asciiData;
	unsigned char asciiLine[512];

	printf("Listening for ASCII messages... (Press Ctrl+C to exit)\n");

	// You can set running to false with some other piece of code to break out of the loop and end the program
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
	
	return 0;
}

