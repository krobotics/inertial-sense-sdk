#pragma once
#include "InertialSense.h"

#ifdef HAS_INERTIALSENSE

#include <chrono>
#include <fstream>
#include <opencv2/core.hpp>

#include "uSleep.h"

// euler angles to rotation matrix from math.h but we need to control the include order here
cv::Matx33d euler2rmat(const cv::Vec3d &a) {
	cv::Matx33d rx = {
		1.0,       0.0,        0.0,
		0.0, cos(a[0]), -sin(a[0]),
		0.0, sin(a[0]),  cos(a[0])
	};
	cv::Matx33d ry = {
		 cos(a[1]), 0.0, sin(a[1]),
		       0.0, 1.0,       0.0,
		-sin(a[1]), 0.0, cos(a[1])
	};
	cv::Matx33d rz = {
		cos(a[2]), -sin(a[2]), 0.0,
		sin(a[2]),  cos(a[2]), 0.0,
		      0.0,        0.0, 1.0
	};
	return rz * ry * rx;
}

int portWrite(unsigned int port, const uint8_t *buf, int len) {
    return IMUStage::stream->Write(buf, len);
}

int IMUStage::set_configuration(cISStream *stream, is_comm_instance_t *comm) {
	// Set INS output Euler rotation in radians to 90 degrees roll for mounting
	float rotation[3] = { 0.0f, 0.0f, 0.0f };
	if ( is_comm_set_data(portWrite, 0, comm, _DID_FLASH_CONFIG, sizeof(float) * 3, offsetof(nvm_flash_cfg_t, insRotation), rotation)) {
		printf("Failed to encode and write set INS rotation\r\n");
		return -3;
	}
	return 0;
}

int IMUStage::stop_message_broadcasting(cISStream *stream, is_comm_instance_t *comm) {
	// Stop all broadcasts on the device
	if (is_comm_stop_broadcasts_all_ports(portWrite, 0, comm) < 0) {
		printf("Failed to encode and write stop broadcasts message\r\n");
		return -3;
	}
	return 0;
}

int IMUStage::save_persistent_messages(cISStream *stream, is_comm_instance_t *comm) {
	system_command_t cfg;
	cfg.command = SYS_CMD_SAVE_PERSISTENT_MESSAGES;
	cfg.invCommand = ~cfg.command;
	if (is_comm_set_data(portWrite, 0, comm, DID_SYS_CMD, sizeof(system_command_t), 0, &cfg)) {
		printf("Failed to write save persistent message\r\n");
		return -3;
	}
	return 0;
}

int IMUStage::enable_message_broadcasting(cISStream *stream, is_comm_instance_t *comm) {
	// Ask for INS message w/ update 4ms period (4ms source period x 1).  Set data rate to zero to disable broadcast and pull a single packet.
	if (is_comm_get_data(portWrite, 0, comm, DID_INS_1, 0, 0, 1) < 0) {
		printf("Failed to encode and write get INS message\r\n");
		return -4;
	}
	// Ask for IMU message at period of 1 (4ms source period x 1).  This could be as high as 1000 times a second (period multiple of 1)
	else if (is_comm_get_data(portWrite, 0, comm, DID_PIMU, 0, 0, 1) < 0) {
		printf("Failed to encode and write get IMU message\r\n");
		return -5;
	}
	return 0;
}

IMUStage::IMUStage(ObjectTracker *parent, int index) {
	parent_ = parent;
	ind = index;
	hadIMU = 0ull;
	hadINS = 0ull;
	iso = IMUStageOutput();
}

void IMUStage::start(std::shared_ptr<IMUCalib> calib) {
	calib_ = calib;
	done_ = false;
	if (calib->file.length() > 0) {
		playback_thread_ = std::thread(&IMUStage::playback, this);
	} else {
		streaming_thread_ = std::thread(&IMUStage::stream, this);
	}
}

void IMUStage::stop() {
	// this callback is called once the pipeline is stopped : it can be initiated by a call
	// to @ref Pipeline::cancel() and/or after all stages are done and all task queues have been cleared
	done_ = true;
	if (playback_thread_.joinable())
		playback_thread_.join();
	if (streaming_thread_.joinable())
		streaming_thread_.join();
}

void IMUStage::playback() {

	using namespace std;
	using namespace chrono;

	long long last_pkt_time_startup = 0;
	IMU_Packet pkt;

	float last_rv[3] = {0.0f, 0.0f, 0.0f};

	imuPlaybackStream.reset(new ifstream(calib_->file, ios::in | ios::binary));

	while (!done_) {
		// read some file if any left
		if (imuPlaybackStream->peek() != EOF) {
			imuPlaybackStream->read((char*)&pkt, IMU_PACKET_SIZE);
		} else {
			first_system_ts = 0;
			this_thread::sleep_for(milliseconds(1));
			//playing = false;
			continue;
		}
				
		// get time
		long long cur_system_ts = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
		cur_packet_ts = pkt.time_startup;

		// translate PKT to ISO
		iso.t = pkt.time_startup;
		iso.dt = pkt.delta_time;

		// TODO: sandbox treats both devices the same but this one is RPY...
		iso.rot_pos[0] = pkt.rpy[2];
		iso.rot_pos[1] = pkt.rpy[1];
		iso.rot_pos[2] = pkt.rpy[0];

		// vel is actually "delta velocity" aka acceleration and NOT in sensor frame m/s^2
		float acc[3] = { 0.0f, 0.0f, 0.0f };
		vectorBodyToReference(pkt.delta_vel, pkt.rpy, acc);
		acc[2] += G_LA * iso.dt;

		iso.tsl_acc[0] = acc[0];
		iso.tsl_acc[1] = acc[1];
		iso.tsl_acc[2] = acc[2];
							
		// integrate acceleration over time into velocity
		iso.tsl_vel[0] += acc[0];
		iso.tsl_vel[1] += acc[1];
		iso.tsl_vel[2] += acc[2];

		// theta is actually "delta theta" aka rotational velocity in absolute radians
		float rv[3] = { 0.0f, 0.0f, 0.0f };
		vectorBodyToReference(pkt.delta_theta, pkt.rpy, rv);
		iso.rot_vel[0] = rv[0];
		iso.rot_vel[1] = rv[1];
		iso.rot_vel[2] = rv[2];

		// discrete differentiation of rot acceleration
		iso.rot_acc[0] = rv[0] - last_rv[0];
		iso.rot_acc[1] = rv[1] - last_rv[1];
		iso.rot_acc[2] = rv[2] - last_rv[2];

		// remember last rot vel
		last_rv[0] = rv[0];
		last_rv[1] = rv[1];
		last_rv[2] = rv[2];

		float toSeconds = 1.0f / pkt.delta_time;
		// rate per second
		iso.tsl_acc[0] *= toSeconds;
		iso.tsl_acc[1] *= toSeconds;
		iso.tsl_acc[2] *= toSeconds;
		iso.rot_vel[0] *= toSeconds;
		iso.rot_vel[1] *= toSeconds;
		iso.rot_vel[2] *= toSeconds;
		iso.rot_acc[0] *= toSeconds;
		iso.rot_acc[1] *= toSeconds;
		iso.rot_acc[2] *= toSeconds;
		// dampen
		iso.tsl_vel[0] *= 0.998f;
		iso.tsl_vel[1] *= 0.998f;
		iso.tsl_vel[2] *= 0.998f;

		// output callbacks
		for (auto cbi = _cbs.begin(); cbi != _cbs.end(); cbi++)
			cbi->cb(ind, iso);

		// compute the offset first, if never done
		if (first_system_ts == 0ll && cur_packet_ts != first_packet_ts) {
			first_system_ts = cur_system_ts;
			first_packet_ts = cur_packet_ts;
			cout << "CalibratedIMU::startStreamingIMX5() - first_system_ts " << first_system_ts << ", first_packet_ts " << first_packet_ts << endl;
		}
		// sleep for remaining time within a few hundred us
		long long expected_ts = first_system_ts + (cur_packet_ts - first_packet_ts);
		if (cur_system_ts < expected_ts) {
			double val = double(expected_ts - cur_system_ts) / double(nanoseconds(seconds(1)).count());
			SleepFor(val);
		}
	}

	// cleanup
	cout << "CalibratedIMU::startStreamingIMX5() - playbackThread exit" << endl;

}

void IMUStage::stream() {

    // STEP 2: Init comm instance
	uint8_t buffer[2048];
	// Initialize the comm instance, sets up state tracking, packet parsing, etc.
	is_comm_init(&comm, buffer, sizeof(buffer));
	
	// STEP 3: Open stream using ISClient
	// Format: "SERIAL:IS:port:baudrate"
	std::string connectionString = "SERIAL:IS:" + calib_.get()->imuSource + ":921600";
	stream = cISClient::OpenConnectionToServer(connectionString);
	if (stream == nullptr) {
		printf("Failed to open stream connection\n");
		return;
	}
	
	// STEP 4: Stop any message broadcasting
	int error;
	if ((error = stop_message_broadcasting(stream, &comm))) {
        printf("Failed to stop_message_broadcasting\n");
        return;
	}
#if 0	// STEP 5: Set configuration
	if ((error = set_configuration(stream, &comm))) {
        printf("Failed to set_configuration\n");
        return;
	}
#endif
	// STEP 6: Enable message broadcasting
	if ((error = enable_message_broadcasting(stream, &comm))) {
        printf("Failed to enable_message_broadcasting\n");
        return;
	}
#if 0   // STEP 7: (Optional) Save currently enabled streams as persistent messages enabled after reboot
	save_persistent_messages(stream, &comm);
#endif
    // STEP 8: Handle received data
	uint8_t inByte;
	uint8_t readBuffer[1];

	float last_rv[3] = { 0.0f, 0.0f, 0.0f };

	// You can set running to false with some other piece of code to break out of the loop and end the program
	while (!done_) {
		// Read one byte at a time
		// Note: ISStream doesn't have a built-in timeout per read like serialPortReadCharTimeout,
		// so we do non-blocking reads with small delays
		int count = stream->Read(readBuffer, 1);
		if (count > 0) {
			inByte = readBuffer[0];
			switch (is_comm_parse_byte(&comm, inByte)) {
				case _PTYPE_INERTIAL_SENSE_DATA:
				switch (comm.rxPkt.dataHdr.id) {
					case DID_INS_1: {

						// (ins_1_t) INS output: euler rotation w/ respect to NED, NED position from reference LLA.
						
						ins_1_t *ins = (ins_1_t*)comm.rxPkt.data.ptr;

						// UVW is always 0... translation velocity meters per second in NED
						//vectorBodyToReference(ins->uvw, ins->theta, iso.tsl_vel);

						// SDK gives rot pos in NED
						iso.rot_pos[0] = ins->theta[0]; // R
						iso.rot_pos[1] = ins->theta[1]; // P
						iso.rot_pos[2] = ins->theta[2]; // Y

						++hadINS;

						break;
					} case DID_PIMU: {

						// (pimu_t) Preintegrated IMU (a.k.a. Coning and Sculling integral) in body/IMU frame.  Updated at IMU rate.
						// Also know as delta theta delta velocity, or preintegrated IMU (PIMU). For clarification, the name "Preintegrated IMU"
						// or "PIMU" throughout our User Manual. This data is integrated from the IMU data at the IMU update rate (startupImuDtMs, default 1ms).
						// The integration period (dt) and output data rate are the same as the NAV rate (startupNavDtMs) and cannot be output at any other rate.
						// If a faster output data rate is desired, DID_IMU_RAW can be used instead. PIMU data acts as a form of compression, adding the benefit
						// of higher integration rates for slower output data rates, preserving the IMU data without adding filter delay and addresses antialiasing.
						// It is most effective for systems that have higher dynamics and lower communications data rates.  The minimum data period is 
						// DID_FLASH_CONFIG.startupImuDtMs or 4, whichever is larger (250Hz max). The PIMU value can be converted to IMU by dividing PIMU by dt (i.e. IMU = PIMU / dt)

						if (hadINS > 1) {

							pimu_t *imu = (pimu_t*)comm.rxPkt.data.ptr;
							float toSeconds = 1.0f / imu->dt;
							iso.t = float(imu->time);
							iso.dt = imu->dt;

							// vel is actually "delta velocity" aka acceleration and NOT in sensor frame m/s^2
							float acc[3] = { 0.0f, 0.0f, 0.0f };
							vectorBodyToReference(imu->vel, iso.rot_pos, acc);
							acc[2] += G_LA * iso.dt;

							iso.tsl_acc[0] = acc[0];
							iso.tsl_acc[1] = acc[1];
							iso.tsl_acc[2] = acc[2];
							
							// integrate acceleration over time into velocity
							iso.tsl_vel[0] += acc[0];
							iso.tsl_vel[1] += acc[1];
							iso.tsl_vel[2] += acc[2];

							// theta is actually "delta theta" aka rotational velocity in absolute radians
							float rv[3] = { 0.0f, 0.0f, 0.0f };
							vectorBodyToReference(imu->theta, iso.rot_pos, rv);
							iso.rot_vel[0] = rv[0];
							iso.rot_vel[1] = rv[1];
							iso.rot_vel[2] = rv[2];

							// discrete differentiation of rot acceleration
							iso.rot_acc[0] = rv[0] - last_rv[0];
							iso.rot_acc[1] = rv[1] - last_rv[1];
							iso.rot_acc[2] = rv[2] - last_rv[2];

							// remember last rot vel
							last_rv[0] = rv[0];
							last_rv[1] = rv[1];
							last_rv[2] = rv[2];

							// wait until both data streams come in to start updating
							if (hadIMU > 1) {
								float toSeconds = 1.0f / imu->dt;
								// rate per second
								iso.tsl_acc[0] *= toSeconds;
								iso.tsl_acc[1] *= toSeconds;
								iso.tsl_acc[2] *= toSeconds;
								iso.rot_vel[0] *= toSeconds;
								iso.rot_vel[1] *= toSeconds;
								iso.rot_vel[2] *= toSeconds;
								iso.rot_acc[0] *= toSeconds;
								iso.rot_acc[1] *= toSeconds;
								iso.rot_acc[2] *= toSeconds;
								// dampen
								iso.tsl_vel[0] *= 0.998f;
								iso.tsl_vel[1] *= 0.998f;
								iso.tsl_vel[2] *= 0.998f;
								// output callbacks
								for (auto cbi = _cbs.begin(); cbi != _cbs.end(); cbi++)
									cbi->cb(ind, iso);
							}

							++hadIMU;
						}
						// they do the rest for us in DID_INS_1
						break;
					} default: {
						break;
					}
				}
			}
		} else if (count == 0) {
			// No data available, sleep briefly to avoid busy-waiting
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		} else {
			// Error reading
			printf("Error reading from stream\n");
			break;
		}
	}
	
	// Cleanup: Close and delete the stream
	if (stream) {
		stream->Close();
		delete stream;
		stream = nullptr;
	}
}

#endif
