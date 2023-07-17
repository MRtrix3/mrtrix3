# TODO: Figure what's the minimum CMake we want
cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(mrtrix3)
include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(BUILD_GUI "Build the MRtrix3 GUI" ON)
option(USE_QT6 "Use Qt6 to build" OFF)
option(WARNING_AS_ERRORS "Compiler warnings result in compilation errors" OFF)
option(ENABLE_STL_DEBUGGING "Enable STL debug mode" OFF)
option(BUILD_TESTS "Build tests executables" ON)

if(BUILD_TESTS)
    enable_testing()
endif()

if(USE_QT6)
    set(QT_MAJOR_VERSION 6)
else()
    set(QT_MAJOR_VERSION 5)
endif()

set(MRTRIX_BASE_VERSION "3.0.4")

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
set(MRTRIX_MAIN_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}
    CACHE STRING "MRtrix3 main binary directory")
    
include(BuildType)
include(FindFFTW)

add_compile_definitions(
    MRTRIX_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    $<$<PLATFORM_ID:Windows>:MRTRIX_WINDOWS>
    $<$<PLATFORM_ID:Darwin>:MRTRIX_MACOSX>
)

if(ENABLE_STL_DEBUGGING AND $<NOT:$<CONFIG:Debug>>)
    if(MSVC)
        add_compile_definitions(_ITERATOR_DEBUG_LEVEL=1)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(_LIBCPP_DEBUG)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(_GLIBCXX_DEBUG _GLIBCXX_DEBUG_PEDANTIC)
    endif()
endif()

if(WARNING_AS_ERRORS)
    if (MSVC)
        add_compile_options(/WX)
    else()
        add_compile_options(-Werror)
    endif()
endif()

add_subdirectory(core)
add_subdirectory(src)
add_subdirectory(cmd)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTS)
    add_subdirectory(testing)
endif()
