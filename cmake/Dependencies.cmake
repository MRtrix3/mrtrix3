include(FetchContent)

# Eigen
if(MRTRIX_USE_SYSTEM_EIGEN)
    find_package(Eigen3 3.4 CONFIG REQUIRED)
else()
    # We avoid configuring the Eigen library via FetchContent_MakeAvailable
    # to avoid the verbosity of Eigen's CMake configuration output.
    set(eigen_url "https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz")
    FetchContent_Declare(
        eigen3
        DOWNLOAD_EXTRACT_TIMESTAMP ON
        URL ${eigen_url}
        # Hack to turn off Eigen's configure
        # See https://stackoverflow.com/questions/77210209/how-to-prevent-eigen-targets-to-show-up-in-the-main-app-in-a-cmake-project
        SOURCE_SUBDIR non_existent_dir
    )

    FetchContent_MakeAvailable(eigen3)

    if(NOT TARGET Eigen3::Eigen)
        add_library(Eigen3 INTERFACE)
        add_library(Eigen3::Eigen ALIAS Eigen3)
        set_target_properties(Eigen3 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${eigen3_SOURCE_DIR})
    endif()
endif()


# Json for Modern C++
if(MRTRIX_USE_SYSTEM_JSON)
    find_package(nlohmann_json 3.11.3 REQUIRED)
else()
    set(json_url "https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz")
    FetchContent_Declare(
        json
        DOWNLOAD_EXTRACT_TIMESTAMP ON
        URL ${json_url}
    )
    FetchContent_MakeAvailable(json)
endif()


# Nifti headers
if(MRTRIX_USE_SYSTEM_NIFTI)
    find_path(NIFTI1_INCLUDE_DIR 
        NAMES nifti1.h
        PATHS ${NIFTI_DIR} /usr/include /usr/local/include
    )
    find_path(NIFTI2_INCLUDE_DIR 
        NAMES nifti2.h
        PATHS ${NIFTI_DIR} /usr/include /usr/local/include
    )
    if(NOT NIFTI1_INCLUDE_DIR)
        message(FATAL_ERROR "Could not find NIFTI1 headers. Please set NIFTI_DIR to the installation prefix.")
    endif()
    if(NOT NIFTI2_INCLUDE_DIR)
        message(FATAL_ERROR "Could not find NIFTI2 headers. Please set NIFTI_DIR to the installation prefix.")
    endif()
    set(NIFTI_INCLUDE_DIRS "${NIFTI1_INCLUDE_DIR};${NIFTI2_INCLUDE_DIR}")
else()
    set(NIFTI1_URL "https://raw.githubusercontent.com/NIFTI-Imaging/nifti_clib/master/nifti2/nifti1.h")
    set(NIFTI2_URL "https://raw.githubusercontent.com/NIFTI-Imaging/nifti_clib/master/nifti2/nifti2.h")
    # DOWNLOAD_NO_EXTRACT for FetchContent is only available in CMake 3.18 and later
    if(CMAKE_VERSION VERSION_LESS 3.18)
        file(DOWNLOAD ${NIFTI1_URL} "${CMAKE_CURRENT_BINARY_DIR}/nifti/nifti1.h")
        file(DOWNLOAD ${NIFTI2_URL} "${CMAKE_CURRENT_BINARY_DIR}/nifti/nifti2.h")
        set(NIFTI_INCLUDE_DIRS "${CMAKE_CURRENT_BINARY_DIR}/nifti")
    else()
        FetchContent_Declare(
            nifti1
            DOWNLOAD_NO_EXTRACT ON
            URL ${NIFTI1_URL}
        )
        FetchContent_Declare(
            nifti2
            DOWNLOAD_NO_EXTRACT ON
            URL ${NIFTI2_URL}
        )
        FetchContent_MakeAvailable(nifti1 nifti2)
        set(NIFTI_INCLUDE_DIRS "${nifti1_SOURCE_DIR};${nifti2_SOURCE_DIR}")
    endif()
endif()

add_library(nifti INTERFACE)
add_library(nifti::nifti ALIAS nifti)

target_include_directories(nifti INTERFACE "${NIFTI_INCLUDE_DIRS}")

# Google Test
if(MRTRIX_BUILD_TESTS)
    set(googletest_version 1.17.0)

    if(MRTRIX_USE_SYSTEM_GTEST)
        find_package(GTest ${googletest_version} REQUIRED)
    else()
        set(googletest_url "https://github.com/google/googletest/releases/download/v${googletest_version}/googletest-${googletest_version}.tar.gz")
        FetchContent_Declare(
            googletest
            DOWNLOAD_EXTRACT_TIMESTAMP ON
            URL ${googletest_url}
        )
        FetchContent_MakeAvailable(googletest)
    endif()
endif()


# Dawn

find_package(Dawn QUIET)

if(NOT Dawn_FOUND)
    message(STATUS "Dawn not found, downloading prebuilt binaries...")
    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(DAWN_PLATFORM "linux")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(DAWN_PLATFORM "macos")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(DAWN_PLATFORM "windows-msys2")
    else()
        message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}")
    endif()

    set(DAWN_VERSION 7495)


    set(DAWN_BINARIES_URL_PREFIX
        "https://github.com/mrtrix3/webgpu-dawn-binaries/releases/download/chromium")
    set(DAWN_BINARIES_URL
        ${DAWN_BINARIES_URL_PREFIX}-${DAWN_VERSION}/webgpu-dawn-chromium-${DAWN_VERSION}-${DAWN_PLATFORM}.zip
    )

    FetchContent_Declare(
        dawn
        DOWNLOAD_EXTRACT_TIMESTAMP   1
        DOWNLOAD_NO_PROGRESS         1
        URL ${DAWN_BINARIES_URL}
    )
    FetchContent_MakeAvailable(dawn)

    if(LINUX)
        set(DAWN_LIB_DIR_NAME "lib64")
    else()
        set(DAWN_LIB_DIR_NAME "lib")
    endif()
    set(
      Dawn_DIR
      "${dawn_SOURCE_DIR}/${DAWN_LIB_DIR_NAME}/cmake/Dawn"
      CACHE PATH "Folder containing DawnConfig.cmake"
      FORCE
    )
    set(FETCHCONTENT_QUIET ON)
endif()


# Slang

find_package(slang QUIET)

if(NOT Slang_FOUND)
    message(STATUS "Slang not found, downloading prebuilt binaries...")
    set(SLANG_VERSION "2025.22.1" CACHE STRING "Slang version to download from GitHub releases")

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(SLANG_OS "linux")
    elseif(APPLE)
        set(SLANG_OS "macos")
    else()
        set(SLANG_OS "windows")
    endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        set(SLANG_ARCH "aarch64")
    else()
        set(SLANG_ARCH "x86_64")
    endif()

    set(SLANG_SUBSTRING "-${SLANG_OS}-${SLANG_ARCH}")

    set(SLANG_DOWNLOAD_LINK
      "https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}/slang-${SLANG_VERSION}${SLANG_SUBSTRING}.zip"
    )

    message(STATUS "Downloading Slang ${SLANG_VERSION} (${SLANG_OS}/${SLANG_ARCH})...")

    FetchContent_Declare(
      slang
        DOWNLOAD_EXTRACT_TIMESTAMP   1
        DOWNLOAD_NO_PROGRESS         1
        URL                          ${SLANG_DOWNLOAD_LINK}
    )
    FetchContent_MakeAvailable(slang)

    set(
      slang_DIR
      "${slang_SOURCE_DIR}/lib/cmake/slang"
      CACHE PATH "Folder containing SlangConfig.cmake"
      FORCE
    )
endif()
