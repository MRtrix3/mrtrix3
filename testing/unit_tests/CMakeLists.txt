set(UNIT_TESTS_SRCS 
    bitset.cpp
    erfinv.cpp
    icls.cpp
    ordered_include.cpp
    ordered_queue.cpp
    parse_ints.cpp
    sh_precomputer.cpp
    shuffle.cpp
    to.cpp
)

find_program(BASH bash)

function(add_unit_test FILE_SRC)
    get_filename_component(NAME ${FILE_SRC} NAME_WE)
    add_executable(${NAME} ${FILE_SRC})
    target_link_libraries(${NAME} PRIVATE
        mrtrix::headless
        mrtrix::exec-version-lib
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
