# TODO: Figure what's the minimum CMake we want
cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

if(NOT DEFINED ${CMAKE_TOOLCHAIN_FILE})
    set(CMAKE_TOOLCHAIN_FILE 
        ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
        CACHE STRING "Vcpkg toolchain file")
endif()

project(mrtrix3)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/Installation)

if(APPLE)
    set(base @loader_path)
else()
    set(base $ORIGIN)
endif()

file(RELATIVE_PATH relDir
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}
)

set(CMAKE_INSTALL_RPATH $base $base/${relDir})

add_compile_definitions(
    MRTRIX_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    $<$<PLATFORM_ID:Windows>:MRTRIX_WINDOWS>
    $<$<PLATFORM_ID:Darwin>:MRTRIX_MACOSX>
)

add_subdirectory(core)
add_subdirectory(src)
add_subdirectory(cmd)
