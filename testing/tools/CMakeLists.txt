file(GLOB TESTING_CMD_SRCS *.cpp)

function(add_testing_cmd CMD_SRC)
    # Extract the filename without an extension (NAME_WE)
    get_filename_component(CMD_NAME ${CMD} NAME_WE)
    add_executable(${CMD_NAME} ${CMD})
    target_link_libraries(${CMD_NAME} PRIVATE
        mrtrix::headless
        exec-version-lib
        mrtrix::tests-lib
    )
    set_target_properties(${CMD_NAME} PROPERTIES LINK_DEPENDS_NO_SHARED true)
endfunction()


foreach(CMD ${TESTING_CMD_SRCS})
    add_testing_cmd(${CMD})
endforeach(CMD)
