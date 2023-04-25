# TODO: Figure what's the minimum CMake we want
cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

if(NOT DEFINED ${CMAKE_TOOLCHAIN_FILE})
    set(CMAKE_TOOLCHAIN_FILE 
        ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
        CACHE STRING "Vcpkg toolchain file")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INSTALL_RPATH "$ORIGIN")

project(mrtrix3)

# TODO: Are these required for the commands?
# If not, we shouldn't add them globally

add_compile_definitions(
    MRTRIX_BUILD_TYPE="release" #TODO: Use CMAKE_BUILD_TYPE
    $<$<PLATFORM_ID:Windows>:MRTRIX_WINDOWS>
    $<$<PLATFORM_ID:Darwin>:MRTRIX_MACOS>
)

add_subdirectory(core)
add_subdirectory(src)
add_subdirectory(cmd)