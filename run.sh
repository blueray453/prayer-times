#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# If --clean is passed, remove the entire build directory
if [ "$1" == "--clean" ]; then
    echo "🧹 Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"

# Configure the project if it hasn't been configured yet
if [ ! -f CMakeCache.txt ]; then
    echo "⚙️  Configuring with CMake..."
    cmake ..
fi

# Build using all available cores
echo "🔨 Building..."
cmake --build . -j

# Run the executable
echo "🚀 Running prayer_times..."
./prayer_times