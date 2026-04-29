#!/bin/bash

# KVStore Build and Run Script for Linux/Mac
# This script builds and runs the complete KVStore project

echo ""
echo "====================================="
echo "    KVStore - Build and Run Script"
echo "====================================="
echo ""

# Check if CMake is installed
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake not found. Please install CMake first."
    echo "On Ubuntu/Debian: sudo apt-get install cmake"
    echo "On Mac: brew install cmake"
    exit 1
fi

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BUILD_DIR="$SCRIPT_DIR/build"

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

# Navigate to build directory
cd "$BUILD_DIR"

# Run CMake
echo ""
echo "[1/4] Running CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed"
    exit 1
fi

# Build the project
echo ""
echo "[2/4] Building project..."
cmake --build . --config Release
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi

# Find the executable
EXE_PATH="$BUILD_DIR/bin/kvstore"
if [ ! -f "$EXE_PATH" ]; then
    EXE_PATH="$BUILD_DIR/kvstore"
fi

if [ ! -f "$EXE_PATH" ]; then
    echo "ERROR: Executable not found"
    exit 1
fi

# Make executable
chmod +x "$EXE_PATH"

# Display build success
echo ""
echo "[3/4] Build successful!"
echo "Executable: $EXE_PATH"
echo ""

# Run the server
echo "[4/4] Starting KVStore Server..."
echo ""
echo "Choose mode:"
echo "  1 - STDIN mode (default - type commands interactively)"
echo "  2 - TCP mode (server on port 6379)"
echo ""
read -p "Enter choice (1 or 2): " choice

case "$choice" in
    2)
        "$EXE_PATH" tcp 6379
        ;;
    *)
        "$EXE_PATH" stdin
        ;;
esac
