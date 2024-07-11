include(FetchContent)

if(NOT ${MRTRIX_DEPENDENCIES_DIR} STREQUAL "")
    message(STATUS "Using local dependencies at ${MRTRIX_DEPENDENCIES_DIR}")
    set(MRTRIX_LOCAL_DEPENDENCIES ON)
else()
    set(MRTRIX_LOCAL_DEPENDENCIES OFF)
endif()

# Eigen
if(MRTRIX_LOCAL_DEPENDENCIES)
    FetchContent_Declare(
        eigen3
        SOURCE_DIR ${MRTRIX_DEPENDENCIES_DIR}/eigen
        UPDATE_DISCONNECTED ON
    )
else()
    FetchContent_Declare(
      eigen3
      GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
      GIT_TAG 3.4.0
      PATCH_COMMAND git apply --ignore-space-change --ignore-whitespace ${PROJECT_SOURCE_DIR}/thirdparty/eigen.patch
      UPDATE_DISCONNECTED ON
    )
endif()
FetchContent_MakeAvailable(eigen3)


# Json for Modern C++
if(MRTRIX_LOCAL_DEPENDENCIES)
    FetchContent_Declare(
        json
        SOURCE_DIR ${MRTRIX_DEPENDENCIES_DIR}/json
        UPDATE_DISCONNECTED ON
    )
else()
    FetchContent_Declare(json
        DOWNLOAD_EXTRACT_TIMESTAMP ON
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
    )
endif()
FetchContent_MakeAvailable(json)


# Half-precision floating-point library
if(MRTRIX_LOCAL_DEPENDENCIES)
    FetchContent_Declare(
        half
        SOURCE_DIR ${MRTRIX_DEPENDENCIES_DIR}/half
        UPDATE_DISCONNECTED ON
    )
else()
    FetchContent_Declare(
        half
        DOWNLOAD_EXTRACT_TIMESTAMP ON
        URL "https://sourceforge.net/projects/half/files/half/2.1.0/half-2.1.0.zip/download"
    )
endif()

FetchContent_MakeAvailable(half)

add_library(half INTERFACE)
add_library(half::half ALIAS half)
target_include_directories(half INTERFACE "${half_SOURCE_DIR}/include")


# Nifti headers
add_library(nifti INTERFACE)
add_library(nifti::nifti ALIAS nifti)
target_include_directories(nifti INTERFACE "${PROJECT_SOURCE_DIR}/thirdparty/nifti")
