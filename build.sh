#!/bin/bash
# ==============================================================================
# Lightweight HRMS - Build Script
# ==============================================================================

set -e

# Add user-space binary and library directories to PATH and LD_LIBRARY_PATH
export PATH="$HOME/.local/bin:$HOME/.local/usr/bin:$PATH"
export LD_LIBRARY_PATH="$HOME/.local/lib:$HOME/.local/usr/lib:$HOME/.local/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH"

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"

TARGET="${1:-release}"

cd "$PROJECT_DIR"

case "$TARGET" in
    clean)
        echo "[build.sh] Cleaning build directory..."
        rm -rf "$BUILD_DIR"
        echo "[build.sh] Clean completed."
        exit 0
        ;;
    debug)
        echo "[build.sh] Configuring and building Debug build (-O0 -g)..."
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASSEMBLY=ON ..
        cmake --build . -j"$(nproc 2>/dev/null || echo 2)"
        ;;
    gui)
        echo "[build.sh] Configuring and building Native GUI (hrms_gui)..."
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_ASSEMBLY=ON ..
        cmake --build . --target hrms_gui -j"$(nproc 2>/dev/null || echo 2)"
        ;;
    local)
        echo "[build.sh] Configuring and building Local Optimized Release (-O3 -march=native)..."
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        cmake -DCMAKE_BUILD_TYPE=Release -DLOCAL_OPTIMIZED=ON -DENABLE_ASSEMBLY=ON ..
        cmake --build . -j"$(nproc 2>/dev/null || echo 2)"
        ;;
    release)
        echo "[build.sh] Configuring and building Portable Release (-O2)..."
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        cmake -DCMAKE_BUILD_TYPE=Release -DLOCAL_OPTIMIZED=OFF -DENABLE_ASSEMBLY=ON ..
        cmake --build . -j"$(nproc 2>/dev/null || echo 2)"
        ;;
    test)
        echo "[build.sh] Building and executing automated test suite..."
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        cmake -DCMAKE_BUILD_TYPE=Release ..
        cmake --build . --target hrms_test -j"$(nproc 2>/dev/null || echo 2)"
        ./hrms_test
        exit 0
        ;;
    *)
        echo "Usage: $0 [release|gui|debug|local|clean|test]"
        exit 1
        ;;
esac

echo "[build.sh] Build successful. Executables available in build/:"
echo " - Native GUI App:   $BUILD_DIR/hrms_gui"
echo " - Terminal TUI:     $BUILD_DIR/hrms"
echo " - Test Suite:       $BUILD_DIR/hrms_test"

