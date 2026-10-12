#!/bin/bash
# ==============================================================================
# Lightweight HRMS - Native Linux GUI Startup Script
# ==============================================================================

set -e

# Add user-space binary and library directories to PATH and LD_LIBRARY_PATH
export PATH="$HOME/.local/bin:$HOME/.local/usr/bin:$PATH"
export LD_LIBRARY_PATH="$HOME/.local/lib:$HOME/.local/usr/lib:$HOME/.local/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH"

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

echo "================================================================================"
echo "         Lightweight HRMS - Native Linux C++20 GUI Application                  "
echo "================================================================================"

cd "$PROJECT_DIR"

# 1. Verify X11 / Wayland Display
if [ -z "$DISPLAY" ] && [ -z "$WAYLAND_DISPLAY" ]; then
    echo "[start_gui.sh] Warning: DISPLAY environment variable is not set."
    echo "[start_gui.sh] Attempting default DISPLAY=:0.0..."
    export DISPLAY=:0.0
fi

# 2. Verify CMake
if ! command -v cmake >/dev/null 2>&1; then
    echo "Error: CMake is not installed or not found in PATH." >&2
    exit 1
fi

# 3. Verify GCC / G++
if ! command -v g++ >/dev/null 2>&1 && ! command -v clang++ >/dev/null 2>&1; then
    echo "Error: C++ compiler (g++ or clang++) not found." >&2
    exit 1
fi

# 4. Create runtime directories
mkdir -p "$PROJECT_DIR/data" "$PROJECT_DIR/logs" "$PROJECT_DIR/reports"

# 5. Build GUI target if executable is missing or build dir not configured
if [ ! -f "$BUILD_DIR/hrms_gui" ]; then
    echo "[start_gui.sh] Building HRMS GUI executable..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    if [ ! -f CMakeCache.txt ]; then
        cmake -DCMAKE_BUILD_TYPE=Release ..
    fi
    cmake --build . --target hrms_gui -j"$(nproc 2>/dev/null || echo 2)"
    cd "$PROJECT_DIR"
fi

# 6. Initialize database if missing
if [ ! -f "$PROJECT_DIR/data/hrms.db" ]; then
    echo "[start_gui.sh] Initializing HRMS database..."
    if command -v sqlite3 >/dev/null 2>&1; then
        sqlite3 "$PROJECT_DIR/data/hrms.db" < "$PROJECT_DIR/database/schema.sql"
        if [ -f "$PROJECT_DIR/database/seed.sql" ]; then
            echo "[start_gui.sh] Seeding initial database data..."
            sqlite3 "$PROJECT_DIR/data/hrms.db" < "$PROJECT_DIR/database/seed.sql"
        fi
    else
        echo "[start_gui.sh] Notice: System sqlite3 CLI not found. Database will be bootstrapped automatically on first startup."
    fi
fi

# 7. Launch HRMS Native GUI
echo "[start_gui.sh] Launching HRMS Native GUI Application..."
exec "$BUILD_DIR/hrms_gui" "$@"
