cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

set(CMAKE_OSX_DEPLOYMENT_TARGET 11.00 CACHE STRING "")
project(mrtrix3 LANGUAGES CXX VERSION 3.0.8)

if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
    message(WARNING "It is recommended to use the Ninja generator to build MRtrix3. "
                    "To use it, run cmake with -G Ninja or set the CMAKE_GENERATOR "
                    "environment variable to Ninja.")
endif()

include(GNUInstallDirs)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Use this project's own version (PROJECT_VERSION), not CMAKE_PROJECT_VERSION, so that the value is
# correct when MRtrix3 is built as a sub-project (e.g. vendored by an external MRtrix3 project).
set(MRTRIX_BASE_VERSION "${PROJECT_VERSION}")

option(MRTRIX_BUILD_GUI "Build the MRtrix3 GUI" ON)
option(MRTRIX_USE_QT5 "Use Qt 5 to build" OFF)
option(MRTRIX_WARNINGS_AS_ERRORS "Compiler warnings result in compilation errors" OFF)
option(MRTRIX_STL_DEBUGGING "Enable STL debug mode" OFF)
option(MRTRIX_BUILD_TESTS "Build tests executables" OFF)
option(MRTRIX_STRIP_CONDA "Strip ananconda/mininconda from PATH to avoid conflicts" ON)
option(MRTRIX_USE_PCH "Use precompiled headers" ON)
option(MRTRIX_PYTHON_SOFTLINK "Build directory softlink to Python source code rather than copying" ON)
option(MRTRIX_BUILD_STATIC "Build MRtrix's library statically" OFF)
option(MRTRIX_USE_LLD "Use lld as the linker" OFF)
option(MRTRIX_ENABLE_GPU "Enable GPU computing features" ON)
option(MRTRIX_IGNORE_VERSION_MISMATCH "Ignore version mismatch between git tag and MRtrix base version" OFF)

option(MRTRIX_USE_SYSTEM_EIGEN "Use system-installed Eigen3 library" OFF)
option(MRTRIX_USE_SYSTEM_JSON "Use system-installed Json for Modern C++ library" OFF)
option(MRTRIX_USE_SYSTEM_NIFTI "Use system-installed NIfTI C headers" OFF)
option(MRTRIX_USE_SYSTEM_GTEST "Use system-installed Google Test library" OFF)
option(MRTRIX_USE_SYSTEM_DAWN "Use system-installed Dawn library" OFF)
option(MRTRIX_USE_SYSTEM_SLANG "Use system-installed Slang library" OFF)
option(MRTRIX_USE_SYSTEM_TCB_SPAN "Use system-installed TCB Span library" OFF)
option(MRTRIX_USE_SYSTEM_MAGIC_ENUM "Use system-installed Magic Enum library" OFF)

if(MRTRIX_BUILD_TESTS)
    list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")
    enable_testing()
endif()

if(MRTRIX_USE_QT5)
    set(QT_MAJOR_VERSION 5)
else()
    set(QT_MAJOR_VERSION 6)
endif()

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

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

include(BuildType)
include(BuildInfo)
include(LinkerSetup)
include(FindFFTW)
include(ECMEnableSanitizers)
include(Dependencies)
include(CompilerCache)
use_compiler_cache()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git AND NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git/hooks/pre-commit)
    message(WARNING
        "pre-commit git hook for clang-format not found!\n"
        "Please install pre-commit (e.g. using `apt install pre-commit` or `pip install pre-commit`) "
        "and then run `pre-commit install` from the ${CMAKE_CURRENT_SOURCE_DIR} directory.")
endif()


if(MRTRIX_STRIP_CONDA AND DEFINED ENV{CONDA_PREFIX})
    message(WARNING "CONDA_PREFIX is set to ${CONDA_PREFIX}, adding conda diretories to CMAKE_IGNORE_PATH to avoid conflicts.\n"
                    "To disable this behaviour, configure CMake with -DMRTRIX_STRIP_CONDA=OFF")
    set(mrtrix_ignore_path_conda
        $ENV{CONDA_PREFIX}/bin $ENV{CONDA_PREFIX}/lib $ENV{CONDA_PREFIX}/include
        $ENV{CONDA_PREFIX} $ENV{CONDA_PREFIX}/Library $ENV{CONDA_PREFIX}/Scripts $ENV{CONDA_PREFIX}/condabin
        $ENV{CONDA_PREFIX}/Library/bin $ENV{CONDA_PREFIX}/Library/lib $ENV{CONDA_PREFIX}/Library/include
    )
    set(CMAKE_IGNORE_PATH "${mrtrix_ignore_path_conda};${CMAKE_IGNORE_PATH}" CACHE STRING "")
endif()

# Check if a compiler or linker shipped with FSL is used
get_filename_component(COMPILER_PATH ${CMAKE_CXX_COMPILER} REALPATH)
get_filename_component(LINKER_PATH ${CMAKE_LINKER} REALPATH)
if(COMPILER_PATH MATCHES "fsl/bin" OR LINKER_PATH MATCHES "fsl/bin")
    message(WARNING "It looks like you are using a compiler or linker shipped with FSL. "
        "This is not recommended, as it may result in unexpected behaviour. "
        "Please consider using the linker and compiler shipped with your system instead."
        "See https://github.com/MRtrix3/mrtrix3/wiki/Building-from-source#compilation-failure-due-to-not-using-system-compiler."
    )
endif()

add_subdirectory(cpp)
add_subdirectory(python)
add_subdirectory(share)

if(PROJECT_IS_TOP_LEVEL AND MRTRIX_BUILD_TESTS)
    add_subdirectory(testing)
endif()

# ------------------------------------------------------------------------------------------------
# Build-tree package export. Enables an external MRtrix3 project to build against this build + source
# tree via find_package(MRtrix3 CONFIG PATHS <this build directory> NO_DEFAULT_PATH). This is a
# build-tree export (absolute paths into this tree), not a relocatable installation package.
# ------------------------------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL AND NOT MRTRIX_BUILD_GUI)
    # Real (non-imported) targets that make up mrtrix::core's public interface.
    set(MRTRIX_EXPORT_TARGETS
        mrtrix-core mrtrix-executable-version mrtrix-version mrtrix-common mrtrix-eigen
        nifti tcb_span)
    # Dependencies that are real targets only when vendored (otherwise system/imported -> find_dependency).
    if(NOT MRTRIX_USE_SYSTEM_JSON)
        list(APPEND MRTRIX_EXPORT_TARGETS nlohmann_json)
    endif()
    if(NOT MRTRIX_USE_SYSTEM_MAGIC_ENUM)
        list(APPEND MRTRIX_EXPORT_TARGETS magic_enum)
    endif()
    if(NOT MRTRIX_USE_SYSTEM_EIGEN)
        list(APPEND MRTRIX_EXPORT_TARGETS Eigen3)
    endif()

    # Export under names that match the existing mrtrix:: aliases (mrtrix::core, not mrtrix::mrtrix-core).
    set_target_properties(mrtrix-core PROPERTIES EXPORT_NAME core)
    set_target_properties(mrtrix-executable-version PROPERTIES EXPORT_NAME executable-version)
    set_target_properties(mrtrix-version PROPERTIES EXPORT_NAME version)
    set_target_properties(mrtrix-common PROPERTIES EXPORT_NAME common)
    set_target_properties(mrtrix-eigen PROPERTIES EXPORT_NAME eigen)

    export(TARGETS ${MRTRIX_EXPORT_TARGETS}
        NAMESPACE mrtrix::
        FILE ${PROJECT_BINARY_DIR}/MRtrix3Targets.cmake)

    # find_dependency() calls the consumer needs for the imported (system) dependencies.
    set(MRTRIX_CONFIG_FIND_DEPS "")
    if(MRTRIX_USE_SYSTEM_EIGEN)
        string(APPEND MRTRIX_CONFIG_FIND_DEPS "find_dependency(Eigen3 3.4)\n")
    endif()
    if(MRTRIX_USE_SYSTEM_JSON)
        string(APPEND MRTRIX_CONFIG_FIND_DEPS "find_dependency(nlohmann_json)\n")
    endif()
    if(MRTRIX_USE_SYSTEM_MAGIC_ENUM)
        string(APPEND MRTRIX_CONFIG_FIND_DEPS "find_dependency(magic_enum)\n")
    endif()
    # PNG is found in a subdirectory, so check mrtrix-core's actual interface rather than PNG_FOUND.
    get_target_property(MRTRIX_CORE_LINKS mrtrix-core INTERFACE_LINK_LIBRARIES)
    if("PNG::PNG" IN_LIST MRTRIX_CORE_LINKS)
        string(APPEND MRTRIX_CONFIG_FIND_DEPS "find_dependency(PNG)\n")
    endif()

    set(MRTRIX_SOURCE_CMAKE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/MRtrix3Config.cmake.in
        ${PROJECT_BINARY_DIR}/MRtrix3Config.cmake
        @ONLY)
endif()

add_custom_target(Github SOURCES
    .github/pull_request_template.md
    .github/ISSUE_TEMPLATE/bug_report.md
    .github/ISSUE_TEMPLATE/custom.md
    .github/ISSUE_TEMPLATE/feature_request.md
    .github/workflows/checks.yml
    .github/workflows/package-linux-tarball.yml
    .github/workflows/package-linux-anaconda.yml
    .github/workflows/package-macos-anaconda.yml
    .github/workflows/package-macos-native.yml
    .github/workflows/package-windows-msys2.yml
    .github/workflows/clang-tidy-review.yml
    .github/workflows/clang-tidy-enforce.yml
)

mrtrix_print_build_instructions()
