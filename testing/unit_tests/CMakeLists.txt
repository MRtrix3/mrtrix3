set(UNIT_TESTS_SRCS 
    testing_unit_tests_bitset.cpp
    testing_unit_tests_erfinv.cpp
    testing_unit_tests_icls.cpp
    testing_unit_tests_ordered_include.cpp
    testing_unit_tests_ordered_queue.cpp
    testing_unit_tests_parse_ints.cpp
    testing_unit_tests_sh_precomputer.cpp
    testing_unit_tests_shuffle.cpp
    testing_unit_tests_to.cpp
)

find_program(BASH bash)

function(add_unit_test FILE_SRC)
    get_filename_component(NAME ${FILE_SRC} NAME_WE)
    add_executable(${NAME} ${FILE_SRC})
    target_link_libraries(${NAME} PRIVATE
        mrtrix::headless
        exec-version-lib
        mrtrix::tests-lib
    )
    set_target_properties(${NAME} PROPERTIES LINK_DEPENDS_NO_SHARED true)
    add_test(
        NAME unittest_${NAME}
        COMMAND ${BASH} -c "$<TARGET_FILE:${NAME}>"
    )
endfunction()


foreach(UNIT_TEST ${UNIT_TESTS_SRCS})
    add_unit_test(${UNIT_TEST})
endforeach()
