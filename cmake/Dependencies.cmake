include(FetchContent)

if(NOT ${MRTRIX_DEPENDENCIES_DIR} STREQUAL "")
    message(STATUS "Using local dependencies at ${MRTRIX_DEPENDENCIES_DIR}")
    set(MRTRIX_LOCAL_DEPENDENCIES ON)
else()
    set(MRTRIX_LOCAL_DEPENDENCIES OFF)
endif()

# Eigen
# We avoid configuring the Eigen library via FetchContent_MakeAvaiable
# to avoid the verbosity of Eigen's CMake configuration output.
if(MRTRIX_LOCAL_DEPENDENCIES)
    set(eigen_url ${MRTRIX_DEPENDENCIES_DIR}/eigen-3.4.0.tar.gz)
else()
    set(eigen_url "https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz")
endif()

FetchContent_Declare(
    eigen3
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    URL ${eigen_url}
)
FetchContent_GetProperties(Eigen3)
if(NOT eigen3_POPULATED)
    FetchContent_Populate(Eigen3)
    add_library(Eigen3 INTERFACE)
    add_library(Eigen3::Eigen ALIAS Eigen3)
    target_include_directories(Eigen3 INTERFACE "${eigen3_SOURCE_DIR}")
endif()

# Json for Modern C++
if(MRTRIX_LOCAL_DEPENDENCIES)
    set(json_url ${MRTRIX_DEPENDENCIES_DIR}/json.tar.xz)
else()
    set(json_url "https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz")
endif()
FetchContent_Declare(
    json
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    URL ${json_url}
)
FetchContent_MakeAvailable(json)


# Half-precision floating-point library
if(MRTRIX_LOCAL_DEPENDENCIES)
    set(half_url ${MRTRIX_DEPENDENCIES_DIR}/half-2.1.0.zip)
else()
    set(half_url "https://sourceforge.net/projects/half/files/half/2.1.0/half-2.1.0.zip/download")
endif()
FetchContent_Declare(
    half
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    URL ${half_url}
)
FetchContent_MakeAvailable(half)

add_library(half INTERFACE)
add_library(half::half ALIAS half)
target_include_directories(half INTERFACE "${half_SOURCE_DIR}/include")


# Nifti headers
add_library(nifti INTERFACE)
add_library(nifti::nifti ALIAS nifti)

if(MRTRIX_LOCAL_DEPENDENCIES)
    target_include_directories(nifti INTERFACE "${MRTRIX_DEPENDENCIES_DIR}/nifti")
else()
    include(ExternalProject)
    ExternalProject_Add(
        nifti1
        PREFIX nifti
        URL "https://nifti.nimh.nih.gov/pub/dist/src/nifti2/nifti1.h"
        CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_COMMAND ""
        DOWNLOAD_NO_EXTRACT ON
        DOWNLOAD_NO_PROGRESS ON
        LOG_DOWNLOAD ON
    )
    ExternalProject_Add(
        nifti2
        PREFIX nifti
        URL "https://nifti.nimh.nih.gov/pub/dist/src/nifti2/nifti2.h"
        CONFIGURE_COMMAND "" BUILD_COMMAND "" INSTALL_COMMAND ""
        DOWNLOAD_NO_EXTRACT ON
        DOWNLOAD_NO_PROGRESS ON
        LOG_DOWNLOAD ON
    )
    target_include_directories(nifti INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/nifti/src/")
endif()

