# TODO: Figure what's the minimum CMake we want
cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(mrtrix3 VERSION 3.0.4)
include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(MRTRIX_BASE_VERSION "${CMAKE_PROJECT_VERSION}")

option(BUILD_GUI "Build the MRtrix3 GUI" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(APPLE)
    set(base @loader_path)
else()
    set(base $ORIGIN)
endif()

file(RELATIVE_PATH relDir
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
)

set(CMAKE_INSTALL_RPATH ${base} ${base}/${relDir})

include(BuildType)
include(FindFFTW)

add_compile_definitions(
    MRTRIX_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    $<$<PLATFORM_ID:Windows>:MRTRIX_WINDOWS>
    $<$<PLATFORM_ID:Darwin>:MRTRIX_MACOS>
)

add_subdirectory(core)
add_subdirectory(src)
add_subdirectory(cmd)