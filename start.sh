#!/bin/bash
# ==============================================================================
# Lightweight HRMS - Startup Script
# ==============================================================================

set -e

# Add user-space binary and library directories to PATH and LD_LIBRARY_PATH
export PATH="$HOME/.local/bin:$HOME/.local/usr/bin:$PATH"
export LD_LIBRARY_PATH="$HOME/.local/lib:$HOME/.local/usr/lib:$LD_LIBRARY_PATH"

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

echo "================================================================================"
echo "                   Lightweight HRMS - Native Linux C++20                        "
echo "================================================================================"

cd "$PROJECT_DIR"

# Verify CMake is available
if ! command -v cmake >/dev/null 2>&1; then
    echo "Error: CMake is not installed or not found in PATH." >&2
    exit 1
fi

# Verify GCC/G++ is available
if ! command -v g++ >/dev/null 2>&1 && ! command -v clang++ >/dev/null 2>&1; then
    echo "Error: C++ compiler (g++ or clang++) not found." >&2
    exit 1
fi

# Create runtime directories
mkdir -p "$PROJECT_DIR/data" "$PROJECT_DIR/logs" "$PROJECT_DIR/reports"

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure CMake if CMakeCache.txt doesn't exist
if [ ! -f CMakeCache.txt ]; then
    echo "[start.sh] Configuring CMake (Release Mode)..."
    cmake -DCMAKE_BUILD_TYPE=Release ..
fi

echo "[start.sh] Building HRMS..."
cmake --build . -j"$(nproc 2>/dev/null || echo 2)"

cd "$PROJECT_DIR"

# Initialize database if missing
if [ ! -f "$PROJECT_DIR/data/hrms.db" ]; then
    echo "[start.sh] Initializing HRMS database..."
    if command -v sqlite3 >/dev/null 2>&1; then
        sqlite3 "$PROJECT_DIR/data/hrms.db" < "$PROJECT_DIR/database/schema.sql"
        if [ -f "$PROJECT_DIR/database/seed.sql" ]; then
            echo "[start.sh] Seeding initial database data..."
            sqlite3 "$PROJECT_DIR/data/hrms.db" < "$PROJECT_DIR/database/seed.sql"
        fi
    else
        echo "[start.sh] Notice: System sqlite3 CLI not found. Database will be bootstrapped automatically on first startup."
    fi
fi

echo "[start.sh] Starting Lightweight HRMS..."
exec "$BUILD_DIR/hrms" "$@"
