#!/bin/bash

# Build script for chat server with automatic cleanup

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo "=== Building Chat Server ==="

# Cleanup old build files
echo "-- Cleaning up old build files..."
if [ -d "${BUILD_DIR}" ]; then
    rm -rf "${BUILD_DIR}"
fi

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "-- Running CMake..."
cmake ..

echo "-- Building with make..."
make -j$(nproc)

echo ""
echo "=== Build complete! ==="
echo "Executable: ${BUILD_DIR}/chat_server"
echo ""
echo "To start the server:"
echo "  ${SCRIPT_DIR}/start_server.sh"
echo "Or:"
echo "  cd ${BUILD_DIR}"
echo "  ./chat_server"