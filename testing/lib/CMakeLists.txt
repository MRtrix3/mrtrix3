add_library(mrtrix-tests-lib INTERFACE)
add_library(mrtrix::tests-lib ALIAS mrtrix-tests-lib)


target_include_directories(mrtrix-tests-lib INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
