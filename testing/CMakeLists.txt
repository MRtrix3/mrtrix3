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
    GIT_TAG 5f87e7b55793756ae940df6a06ee3b2a39483b85
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

add_subdirectory(binaries)
add_subdirectory(integration_tests)
add_subdirectory(lib)
add_subdirectory(scripts)
add_subdirectory(tools)
add_subdirectory(ui_tests)
add_subdirectory(unit_tests)
