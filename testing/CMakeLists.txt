if(NOT DEFINED ENV{MRTRIX_BINARIES_DATA_DIR})
    set(mrtrix_binaries_data_url "https://github.com/mrtrix3/test_data.git")
else()
    set(mrtrix_binaries_data_url $ENV{MRTRIX_BINARIES_DATA_DIR})
    message(STATUS "Using binaries data from $ENV{MRTRIX_BINARIES_DATA_DIR}")
endif()

if(NOT DEFINED ENV{MRTRIX_SCRIPTS_DATA_DIR})
    set(mrtrix_scripts_data_url "https://github.com/mrtrix3/script_test_data.git")
else()
    set(mrtrix_scripts_data_url $ENV{MRTRIX_SCRIPTS_DATA_DIR})
    message(STATUS "Using scripts data from $ENV{MRTRIX_SCRIPTS_DATA_DIR}")
endif()

include(ExternalProject)
ExternalProject_Add(BinariesTestData
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/binaries_data
    GIT_REPOSITORY ${mrtrix_binaries_data_url}
    GIT_TAG b9b562018253402de0432f942d850e55f951b613
    GIT_PROGRESS TRUE
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Add(ScriptsTestData
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/scripts_data
    GIT_REPOSITORY ${mrtrix_scripts_data_url}
    GIT_TAG 17e0f19411656481ded3f524042785cf53e39ac9
    GIT_PROGRESS TRUE
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)

set(BINARY_DATA_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/binaries_data/src/BinariesTestData
)

set(SCRIPT_DATA_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/scripts_data/src/ScriptsTestData
)

# Reference data published by the originators of externally-defined tractogram file
# formats, used to validate MRtrix3's readers against canonical exemplars. Each
# dataset is downloaded at build time into an independent location and exposed as a
# variable so that individual tests can nominate it as their working directory.

# Adopt the recommended timestamp behaviour for downloaded archives where the host
# CMake is new enough to offer it, suppressing the CMP0135 configure-time warning.
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# TRX format gold-standard data, distributed as a single archive that ExternalProject
# extracts into the source directory.
ExternalProject_Add(TRXReferenceData
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/trx_data
    URL "https://github.com/tee-ar-ex/trx-test-data/releases/download/v0.1.0/gold_standard.zip"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)

# .trk (and companion .tck / .nii.gz) reference data from NiBabel; only the subset of
# files relevant to MRtrix3's format tests is fetched.
set(trk_reference_base_url
    "https://raw.githubusercontent.com/nipy/nibabel/5.4.2/nibabel/tests/data"
)
string(JOIN "|" trk_reference_files
    simple.tck
    simple.trk
    simple_big_endian.tck
    standard.LPS.trk
    standard.nii.gz
    standard.tck
    standard.trk
)
ExternalProject_Add(TRKReferenceData
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/trk_data
    DOWNLOAD_COMMAND ${CMAKE_COMMAND}
        -D BASE_URL=${trk_reference_base_url}
        -D FILES=${trk_reference_files}
        -D DEST_DIR=<SOURCE_DIR>
        -P ${PROJECT_SOURCE_DIR}/cmake/DownloadFiles.cmake
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

# .vtk / .vtx (and companion .tck / .trk) reference data from rordenlab; only the
# subset of files relevant to MRtrix3's format tests is fetched.
set(vtk_reference_base_url
    "https://raw.githubusercontent.com/rordenlab/TractographyFormat/master/DATA"
)
string(JOIN "|" vtk_reference_files
    example.tck
    example.trk
    example.vtk
    example.vtx
)
ExternalProject_Add(VTKReferenceData
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/vtk_data
    DOWNLOAD_COMMAND ${CMAKE_COMMAND}
        -D BASE_URL=${vtk_reference_base_url}
        -D FILES=${vtk_reference_files}
        -D DEST_DIR=<SOURCE_DIR>
        -P ${PROJECT_SOURCE_DIR}/cmake/DownloadFiles.cmake
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

# Locations of the downloaded (and, for TRX, extracted) reference data, for tests to
# nominate as their working directory.
set(TRX_DATA_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/trx_data/src/TRXReferenceData
)

set(TRK_DATA_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/trk_data/src/TRKReferenceData
)

set(VTK_DATA_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/vtk_data/src/VTKReferenceData
)

add_subdirectory(binaries)
add_subdirectory(integration_tests)
add_subdirectory(lib)
add_subdirectory(scripts)
add_subdirectory(tools)
add_subdirectory(ui_tests)
add_subdirectory(unit_tests)
