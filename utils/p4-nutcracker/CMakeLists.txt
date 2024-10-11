cmake_minimum_required(VERSION 3.10)
project(p4_nutcracker)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define paths
set(SRC_DIR "${CMAKE_SOURCE_DIR}")

# Include Abseil package
find_package(absl REQUIRED)

# Collect all source files, explicitly excluding CMake build files
file(GLOB_RECURSE CXX_SRC "${SRC_DIR}/*.cpp")
list(FILTER CXX_SRC EXCLUDE REGEX ".*CMakeFiles.*")

# Create an executable target
add_executable(p4_nutcracker ${CXX_SRC})

# Specify include directories
target_include_directories(p4_nutcracker PRIVATE 
    ${absl_INCLUDE_DIRS}
    ${SRC_DIR}
)

# Link Abseil libraries
target_link_libraries(p4_nutcracker PRIVATE 
    absl::status
    absl::strings
    absl::cord
)

# Add compile options
target_compile_options(p4_nutcracker PRIVATE 
    -Wall
    -O3
    -g
)

# Specify the C++ standard
set_property(TARGET p4_nutcracker PROPERTY CXX_STANDARD 17)
