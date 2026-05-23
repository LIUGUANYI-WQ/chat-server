#!/bin/bash

# Start script for chat client

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
EXECUTABLE="${BUILD_DIR}/client"

echo "=== Starting Chat Client ==="

# Check if executable exists
if [ ! -f "${EXECUTABLE}" ]; then
    echo "Error: Executable not found at ${EXECUTABLE}"
    echo "Please run build.sh first"
    exit 1
fi

cd "${BUILD_DIR}"
exec "./client"