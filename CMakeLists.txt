cmake_minimum_required(VERSION 3.10)
project(FBXProcessor)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify the path to the FBX SDK
# Replace this path with the location of your FBX SDK installation
set(FBX_SDK_ROOT "/usr/fbx202037_fbxsdk_linux/")

# Include directories for the FBX SDK
include_directories("${FBX_SDK_ROOT}/include")

# Link directories for the FBX SDK libraries
link_directories("${FBX_SDK_ROOT}/lib/release")  # Modify for your compiler and platform

# Define the executable
add_executable(FBXProcessor main.cpp)

# Link the FBX SDK libraries
target_link_libraries(FBXProcessor fbxsdk)

# Set runtime library paths (optional, for Linux/macOS)
if(UNIX AND NOT APPLE)
    set(CMAKE_BUILD_RPATH "${FBX_SDK_ROOT}/lib")
endif()

# Output message
message(STATUS "Using FBX SDK from: ${FBX_SDK_ROOT}")
