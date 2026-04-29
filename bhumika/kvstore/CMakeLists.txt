cmake_minimum_required(VERSION 3.10)
project(KVStore)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/kvstore.cpp
    src/command_parser.cpp
)

# Create executable
add_executable(kvstore ${SOURCES})

# Link libraries
if(WIN32)
    target_link_libraries(kvstore ws2_32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(kvstore Threads::Threads)
endif()

# Set output directory
set_target_properties(kvstore PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
