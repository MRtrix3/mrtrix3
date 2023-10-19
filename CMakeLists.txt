cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(mrtrix3 VERSION 3.0.4)
include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(MRTRIX_BASE_VERSION "${CMAKE_PROJECT_VERSION}")

option(MRTRIX_BUILD_GUI "Build the MRtrix3 GUI" ON)
option(MRTRIX_USE_QT6 "Use Qt6 to build" OFF)
option(MRTRIX_WARNINGS_AS_ERRORS "Compiler warnings result in compilation errors" OFF)
option(MRTRIX_STL_DEBUGGING "Enable STL debug mode" OFF)
option(MRTRIX_BUILD_TESTS "Build tests executables" OFF)

if(MRTRIX_BUILD_TESTS)
    if(CMAKE_VERSION VERSION_GREATER 3.17)
        list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")
    endif()
    enable_testing()
endif()

if(MRTRIX_USE_QT6)
    set(QT_MAJOR_VERSION 6)
else()
    set(QT_MAJOR_VERSION 5)
endif()

option(BUILD_GUI "Build the MRtrix3 GUI" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(CMAKE_VERSION VERSION_LESS 3.21)
  get_property(not_top DIRECTORY PROPERTY PARENT_DIRECTORY)
  if(NOT not_top)
    set(PROJECT_IS_TOP_LEVEL true)
  endif()
endif()

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
include(CompilerCache)

use_compiler_cache()

add_compile_definitions(
    MRTRIX_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    $<$<PLATFORM_ID:Windows>:MRTRIX_WINDOWS>
    $<$<PLATFORM_ID:Darwin>:MRTRIX_MACOSX>
)

if(MRTRIX_STL_DEBUGGING AND $<NOT:$<CONFIG:Debug>>)
    if(MSVC)
        add_compile_definitions(_ITERATOR_DEBUG_LEVEL=1)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(_LIBCPP_DEBUG)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(_GLIBCXX_DEBUG _GLIBCXX_DEBUG_PEDANTIC)
    endif()
endif()

if(MRTRIX_WARNINGS_AS_ERRORS)
    if (MSVC)
        add_compile_options(/WX)
    else()
        add_compile_options(-Werror)
    endif()
endif()

add_subdirectory(cmd)
add_subdirectory(core)
add_subdirectory(python)
add_subdirectory(share)
add_subdirectory(src)

if(PROJECT_IS_TOP_LEVEL AND MRTRIX_BUILD_TESTS)
    add_subdirectory(testing)
endif()

add_custom_target(Github SOURCES
    .github/pull_request_template.md
    .github/ISSUE_TEMPLATE/bug_report.md
    .github/ISSUE_TEMPLATE/custom.md
    .github/ISSUE_TEMPLATE/feature_request.md
    .github/workflows/checks.yml
    .github/workflows/package-linux-anaconda.yml
    .github/workflows/package-macos-anaconda.yml
    .github/workflows/package-macos-native.yml
    .github/workflows/package-windows-msys2.yml
)
