add_library(mrtrix-tests-lib STATIC
    unit_tests/unit_test.h
    unit_tests/tractography/roi_unit_tests.h
    diff_images.h
)
add_library(mrtrix::tests-lib ALIAS mrtrix-tests-lib)

set_target_properties(mrtrix-tests-lib PROPERTIES
    LINKER_LANGUAGE CXX
)

target_include_directories(mrtrix-tests-lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
