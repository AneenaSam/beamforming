cmake_minimum_required(VERSION 3.20)
project(CircularArrayBeamforming VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler optimization and warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Core Signal Processing Sources
set(BEAMFORMING_SOURCES
    src/circular_array.cpp
    src/signal_generator.cpp
    src/steering_vector.cpp
    src/covariance.cpp
    src/bartlett.cpp
)

# Main Executable
add_executable(beamforming_app
    src/main.cpp
    ${BEAMFORMING_SOURCES}
)

# Geometry Unit Test
add_executable(test_geometry
    tests/test_geometry.cpp
    src/circular_array.cpp
)

# Output data directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data)
