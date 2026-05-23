#!/bin/bash

# Start script for chat server

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
EXECUTABLE="${BUILD_DIR}/chat_server"

echo "=== Starting Chat Server ==="

# Check if executable exists
if [ ! -f "${EXECUTABLE}" ]; then
    echo "Error: Executable not found at ${EXECUTABLE}"
    echo "Please run build.sh first"
    exit 1
fi

# Check if MySQL is running
if ! mysqladmin ping -h127.0.0.1 -uroot -p123456 --silent; then
    echo "Warning: MySQL may not be running"
fi

# Check if Redis is running
if ! redis-cli ping | grep -q "PONG"; then
    echo "Warning: Redis may not be running"
fi

echo "-- Starting server on port 8888..."
cd "${BUILD_DIR}"
exec "./chat_server"