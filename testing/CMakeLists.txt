include(ExternalProject)
ExternalProject_Add(BinariesTestData
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/binaries_data
    GIT_REPOSITORY https://github.com/MRtrix3/test_data.git
    GIT_TAG f6e117e99481331d0bc39685b4aa122844ca885e
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
)

ExternalProject_Add(ScriptsTestData
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/scripts_data
    GIT_REPOSITORY https://github.com/MRtrix3/script_test_data.git
    GIT_TAG 4f0d6653e994212d3653660a4f9163437956c989
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
add_subdirectory(lib)
add_subdirectory(scripts)
add_subdirectory(tools)
add_subdirectory(unit_tests)