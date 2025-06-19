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
    set(NIFTI1_URL "https://raw.githubusercontent.com/NIFTI-Imaging/nifti_clib/master/nifti2/nifti1.h")
    set(NIFTI2_URL "https://raw.githubusercontent.com/NIFTI-Imaging/nifti_clib/master/nifti2/nifti2.h")
    # DOWNLOAD_NO_EXTRACT for FetchContent is only available in CMake 3.18 and later
    if(CMAKE_VERSION VERSION_LESS 3.18)
        file(DOWNLOAD ${NIFTI1_URL} "${CMAKE_CURRENT_BINARY_DIR}/nifti/nifti1.h")
        file(DOWNLOAD ${NIFTI2_URL} "${CMAKE_CURRENT_BINARY_DIR}/nifti/nifti2.h")
        target_include_directories(nifti INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/nifti")
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
        target_include_directories(nifti INTERFACE
            "${nifti1_SOURCE_DIR}"
            "${nifti2_SOURCE_DIR}"
        )
    endif()
endif()

# Google Test
set(googletest_version 1.17.0)
if(MRTRIX_LOCAL_DEPENDENCIES)
    set(googletest_url ${MRTRIX_DEPENDENCIES_DIR}/googletest-${googletest_version}.tar.gz)
else()
    set(googletest_url "https://github.com/google/googletest/releases/download/v${googletest_version}/googletest-${googletest_version}.tar.gz")
endif()

FetchContent_Declare(
    googletest
    DOWNLOAD_EXTRACT_TIMESTAMP ON
    URL ${googletest_url}
)
FetchContent_MakeAvailable(googletest)

