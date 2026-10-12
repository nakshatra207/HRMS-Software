cmake_minimum_required(VERSION 3.16)
project(hrms LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# User-space search paths (~/.local)
list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.local" "$ENV{HOME}/.local/usr")

# Build configuration options
option(ENABLE_ASSEMBLY "Enable x86-64 NASM Assembly optimizations" ON)
option(LOCAL_OPTIMIZED "Enable -march=native -O3 optimizations" OFF)

# Compiler hardening and optimization flags
set(COMMON_FLAGS "-Wall -Wextra -Wpedantic -pipe")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -O0 -g")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${COMMON_FLAGS} -O0 -g")
elseif(LOCAL_OPTIMIZED)
    message(STATUS "Configuring Local Optimized Release (-O3 -march=native)")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -O3 -march=native -DNDEBUG")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${COMMON_FLAGS} -O3 -march=native -DNDEBUG")
else()
    message(STATUS "Configuring Portable Release (-O2 -fstack-protector-strong)")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -O2 -fstack-protector-strong -DNDEBUG")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   ${COMMON_FLAGS} -O2 -fstack-protector-strong -DNDEBUG")
endif()

# Find SQLite3 using standard hints
find_path(SQLite3_INCLUDE_DIR NAMES sqlite3.h
    PATHS "$ENV{HOME}/.local/include" "$ENV{HOME}/.local/usr/include" /usr/include /usr/local/include
)
find_library(SQLite3_LIBRARY NAMES sqlite3
    PATHS "$ENV{HOME}/.local/lib" "$ENV{HOME}/.local/usr/lib" /usr/lib /usr/local/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SQLite3
    REQUIRED_VARS SQLite3_LIBRARY SQLite3_INCLUDE_DIR
)

if(NOT TARGET SQLite::SQLite3)
    add_library(SQLite::SQLite3 UNKNOWN IMPORTED)
    set_target_properties(SQLite::SQLite3 PROPERTIES
        IMPORTED_LOCATION "${SQLite3_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SQLite3_INCLUDE_DIR}"
    )
endif()

find_package(Threads REQUIRED)

# NASM Assembly Detection
set(ASM_SOURCES "")
if(ENABLE_ASSEMBLY)
    find_program(NASM_EXECUTABLE nasm PATHS "$ENV{HOME}/.local/bin" "$ENV{HOME}/.local/usr/bin" /usr/bin /usr/local/bin)
    if(NASM_EXECUTABLE)
        message(STATUS "NASM found: ${NASM_EXECUTABLE}. Enabling x86-64 Assembly optimizations.")
        enable_language(ASM_NASM)
        set(CMAKE_ASM_NASM_FLAGS "-f elf64")
        set(ASM_SOURCES asm/x86_64/fast_ops.asm)
        add_compile_definitions(HRMS_HAS_ASM)
    else()
        message(STATUS "NASM not found. Building with pure C/C++ fallbacks.")
    endif()
else()
    message(STATUS "Assembly optimizations explicitly disabled. Using pure C/C++ fallbacks.")
endif()

# Core HRMS Sources
set(HRMS_LIB_SOURCES
    # C lowlevel
    c/lowlevel/mem_utils.c
    c/lowlevel/posix_sys.c
    c/lowlevel/sha256.c

    # C++ Core & Utilities
    src/core/config.cpp
    src/utilities/logger.cpp
    src/utilities/fast_ops.cpp
    src/utilities/common_utils.cpp

    # Database
    src/database/statement.cpp
    src/database/transaction.cpp
    src/database/database.cpp

    # Security
    src/security/crypto.cpp
    src/security/rbac.cpp
    src/security/auth.cpp
    src/security/audit.cpp

    # HR Modules
    src/employees/department.cpp
    src/employees/employee.cpp
    src/attendance/attendance.cpp
    src/leave/leave.cpp
    src/payroll/payroll.cpp
    src/recruitment/recruitment.cpp
    src/reports/reports.cpp

    # UI
    src/core/ui.cpp

    # Assembly (if enabled)
    ${ASM_SOURCES}
)

# Include Directories
include_directories(
    include
    c/lowlevel
    ${SQLite3_INCLUDE_DIR}
    "$ENV{HOME}/.local/usr/include"
    "$ENV{HOME}/.local/usr/include/cairo"
    "$ENV{HOME}/.local/usr/include/freetype2"
    /usr/include/cairo
    /usr/include/freetype2
)

# Link Directories for GUI libraries
link_directories(
    "$ENV{HOME}/.local/lib"
    "$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu"
    /usr/lib/x86_64-linux-gnu
)

# Shared Core Static Library
add_library(hrms_core STATIC ${HRMS_LIB_SOURCES})
target_link_libraries(hrms_core PUBLIC SQLite::SQLite3 Threads::Threads)

# Main HRMS TUI Executable
add_executable(hrms src/main.cpp)
target_link_libraries(hrms PRIVATE hrms_core)

# Automated Test Suite Executable
add_executable(hrms_test tests/test_hrms.cpp)
target_link_libraries(hrms_test PRIVATE hrms_core)

# HRMS Native GUI Executable Sources
set(HRMS_GUI_SOURCES
    src/gui/main_gui.cpp
    src/gui/gui_theme.cpp
    src/gui/gui_common.cpp
    src/gui/dialog_login.cpp
    src/gui/tab_dashboard.cpp
    src/gui/tab_employees.cpp
    src/gui/tab_attendance.cpp
    src/gui/tab_leave.cpp
    src/gui/tab_payroll.cpp
    src/gui/tab_recruitment.cpp
    src/gui/tab_departments.cpp
    src/gui/tab_reports.cpp
    src/gui/tab_settings.cpp
    src/gui/gui_app.cpp
)

add_executable(hrms_gui ${HRMS_GUI_SOURCES})
target_link_libraries(hrms_gui PRIVATE
    hrms_core
    fltk
    fltk_images
    cairo
    fontconfig
    freetype
    X11
    Xext
    Xrender
    png
    m
)

# Target Properties
set_target_properties(hrms PROPERTIES OUTPUT_NAME "hrms")
set_target_properties(hrms_test PROPERTIES OUTPUT_NAME "hrms_test")
set_target_properties(hrms_gui PROPERTIES
    OUTPUT_NAME "hrms_gui"
    BUILD_RPATH "$ORIGIN/../lib:$ORIGIN/../usr/lib/x86_64-linux-gnu:$ENV{HOME}/.local/lib:$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu"
    INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/../usr/lib/x86_64-linux-gnu:$ENV{HOME}/.local/lib:$ENV{HOME}/.local/usr/lib/x86_64-linux-gnu"
)
