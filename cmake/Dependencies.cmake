include(FetchContent)

if(NOT ${MRTRIX_DEPENDENCIES_DIR} STREQUAL "")
    message(STATUS "Using local dependencies at ${MRTRIX_DEPENDENCIES_DIR}")
    set(MRTRIX_LOCAL_DEPENDENCIES ON)
else()
    set(MRTRIX_LOCAL_DEPENDENCIES OFF)
endif()

# Eigen
if(MRTRIX_LOCAL_DEPENDENCIES)
    set(eigen_url ${MRTRIX_DEPENDENCIES_DIR}/eigen-3.4.0.tar.gz)
else()
    set(eigen_url "https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz")
endif()
FetchContent_MakeAvailable(eigen3)

FetchContent_Declare(
    eigen3
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    URL ${eigen_url}
)

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
target_include_directories(nifti INTERFACE "${PROJECT_SOURCE_DIR}/thirdparty/nifti")
