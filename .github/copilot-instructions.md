# Inertial Sense SDK - GitHub Copilot Instructions

## Project Overview

The Inertial Sense SDK is an open-source software development kit for communication with Inertial Sense products (uINS, uAHRS, uIMU). This repository includes:

- **SDK Library**: Core C/C++ library for device communication and data processing
- **CLTool**: Command-line utility for device communication, logging, and firmware updates
- **LogInspector**: Python-based tool for analyzing logged data
- **ROS/ROS2 Wrappers**: Integration nodes for Robot Operating System
- **Example Projects**: Demonstration projects showing SDK usage
- **EVB-2**: Multi-purpose evaluation and development kit support

## Technology Stack

- **Languages**: C (C11), C++ (C++20), Python 3
- **Build System**: CMake (minimum version 3.10.0)
- **Testing**: Google Test (gtest)
- **Dependencies**: libusb, udev (Linux), ZMQ (optional), yaml-cpp
- **Platforms**: Linux and Windows
- **ROS**: ROS 1 (Noetic) and ROS 2 (Jazzy) support

## Code Style and Standards

### Formatting
- Follow Google C++ Style Guide with customizations defined in `.clang-format`
- Column limit: 200 characters
- Indentation: 4 spaces (never use tabs)
- Align consecutive macros across empty lines and comments
- Insert newline at end of file
- Case block labels are not indented (IndentCaseBlocks: false)

### Conventions
- Use modern C++20 features where appropriate
- Maintain C11 compatibility for C code
- Follow existing naming conventions in the codebase
- Include proper error handling and validation
- Add comments for complex logic, but prefer self-documenting code

## Building and Testing

### Linux Build Commands

```bash
# Install dependencies
./scripts/install_sdk_dependencies.sh
./scripts/install_gtest.sh

# Clean build artifacts
./scripts/clean_all.sh

# Build SDK library
./scripts/build_is_sdk.sh

# Build and run unit tests
./scripts/build_unit_tests.sh
./scripts/build_unit_tests.sh --nobuild --test

# Build CLTool
./scripts/build_cltool.sh

# Build LogInspector
./scripts/build_log_inspector.sh

# Build SDK examples
cd scripts
python3 build_manager.py SDK_Examples ../ExampleProjects
```

### Windows Build Commands

```bash
# From scripts/windows directory
./clean_all.bat
./build_is_sdk.bat
./build_unit_tests.bat
./build_cltool.bat
./build_log_inspector.bat
./build_sdk_examples.bat
```

### ROS Build Commands

```bash
# ROS 1 (from ROS/catkin_ws)
source /opt/ros/noetic/setup.bash
catkin_make -DCATKIN_BLACKLIST_PACKAGES="inertial_sense_ros2"
catkin_make run_tests

# ROS 2 (from ROS/ros2_ws)
colcon build
source install/setup.bash
ros2 run inertial_sense_ros2 test_inertial_sense_ros2
```

### Testing
- Always run unit tests after making code changes
- Unit tests are located in the `tests/` directory
- Use gtest framework for new tests
- Verify both Linux and Windows builds when possible
- ROS bridge tests require actual hardware

## Project Structure

```
├── src/                    # Core SDK source code
│   ├── protocol/          # Communication protocol implementation
│   ├── util/              # Utility functions and helpers
│   ├── libusb/            # USB communication library
│   └── yaml-cpp/          # YAML parsing library
├── tests/                 # Unit tests (gtest-based)
├── cltool/                # Command-line tool
├── python/                # Python utilities and LogInspector
├── ROS/                   # ROS 1 and ROS 2 wrappers
├── ExampleProjects/       # SDK usage examples
├── hw-libs/               # Hardware-specific libraries
├── scripts/               # Build and utility scripts
└── docs/                  # Documentation

```

## Key Files to Understand

- `CMakeLists.txt`: Main build configuration
- `src/ISComm.h/cpp`: Core communication interface
- `src/InertialSense.h/cpp`: Main SDK interface class
- `src/data_sets.h`: Data structure definitions for device communication
- `cltool/cltool_main.cpp`: Example of SDK usage in a real application

## Development Guidelines

### Making Changes
1. Always check existing code patterns before implementing new features
2. Run the linter to ensure code style compliance (see `.github/workflows/cpp-linter.yml`)
3. Build and test on Linux (at minimum) before submitting changes
4. Update relevant documentation when adding new features
5. Add unit tests for new functionality when appropriate

### Common Tasks

#### Adding New Data Types
- Define structures in `src/data_sets.h`
- Update data mappings in `src/ISDataMappings.cpp`
- Add serialization/deserialization support
- Update protocol version if necessary

#### Adding New Features to SDK
- Implement in appropriate `src/` file
- Add corresponding header declarations
- Update CMakeLists.txt if new files are added
- Add unit tests in `tests/`
- Update example projects if relevant

#### Modifying Communication Protocol
- Changes must maintain backward compatibility when possible
- Update protocol version numbers
- Test with actual hardware when available
- Update both C and C++ implementations

### Dependencies and Libraries
- Minimize external dependencies
- yaml-cpp and libusb are compiled from source (included in repo)
- ZMQ support is optional
- Use standard library features when possible

## Continuous Integration

The project uses GitHub Actions for CI with the following workflows:

1. **main.yml**: Builds and tests SDK on Linux and Windows, plus ROS bridge tests
2. **cpp-linter.yml**: Checks C++ code style compliance

All PRs should pass CI checks before merging.

## Important Notes

- The SDK supports multiple Inertial Sense device types (uINS, uAHRS, uIMU, EVB-2)
- Real-time performance is critical for some applications
- Thread safety should be considered for multi-threaded applications
- Bootloader code has special requirements and should be modified carefully
- ROS bridge tests require actual hardware and run on self-hosted runners
- Maintain cross-platform compatibility (Linux/Windows)

## License

MIT License - Copyright 2014-2025 Inertial Sense, Inc.

## Support

For questions or issues, contact support@inertialsense.com or file an issue on GitHub.
