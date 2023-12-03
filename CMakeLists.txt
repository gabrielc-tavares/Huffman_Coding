# Adjust the minimum required CMake version
cmake_minimum_required(VERSION 3.18)

project(Huffman)

# Define a C++ standard
set(CMAKE_CXX_STANDARD 20)

# Set build type to Release by default
# if(NOT CMAKE_BUILD_TYPE)
#     set(CMAKE_BUILD_TYPE Release)
# endif()

# Configure Boost
# set(BOOST_ROOT "C:/boost")
# set(Boost_USE_STATIC_LIBS OFF) 
# set(Boost_USE_MULTITHREADED ON)  
# set(Boost_USE_STATIC_RUNTIME OFF) 
# set(Boost_ADDITIONAL_VERSIONS "1.69.0" "1.69")
# find_package(boost REQUIRED COMPONENTS system)

# Add your source files here
set(SOURCES
    "src/pathmgr.h"
    "src/pathmgr.cpp"
    "src/raii_handle.hpp"
    "src/huffman_tree.hpp"
    "src/hzip.h"
    "src/hzip.cpp"
    "src/main.cpp"
)

# Create the executable
add_executable(Huffman ${SOURCES})

# Output directory for the executable
set_target_properties(Huffman PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# Include Boost path
# target_include_directories(Huffman PUBLIC "C:/boost/boost_1_83_0") 
# target_link_libraries(Huffman PUBLIC "C:/boost")

# Ensure the C++ standard is used for the executable
target_compile_features(Huffman PRIVATE cxx_std_20)