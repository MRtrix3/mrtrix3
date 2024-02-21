# A function that adds a bash test for each line in a given file
function(add_bash_tests)
    set(singleValueArgs FILE_PATH PREFIX WORKING_DIRECTORY)
    set(multiValueArgs EXEC_DIRECTORIES)
    cmake_parse_arguments(
        ARG
        ""
        "${singleValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    set(file_path ${ARG_FILE_PATH})
    set(prefix ${ARG_PREFIX})
    set(working_directory ${ARG_WORKING_DIRECTORY})
    set(exec_directories ${ARG_EXEC_DIRECTORIES})

    # In MINGW, we need to replace paths starting with drive:/ with /drive/
    # when invoking a bash command (e.g. using bash -c "command")
    if(MINGW AND WIN32)
        foreach(exec_dir ${exec_directories})
            EXECUTE_PROCESS(
                COMMAND cygpath -u ${exec_dir}
                OUTPUT_VARIABLE new_exec_dir
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            list(REMOVE_ITEM exec_directories ${exec_dir})
            list(APPEND exec_directories ${new_exec_dir})
        endforeach()
    endif()

    get_filename_component(file_name ${file_path} NAME_WE)

    # Add a custom target for IDEs to pickup the test script
    add_custom_target(test_${file_name} SOURCES ${file_path})

    # Add test that cleans up temporary files
    add_test(
        NAME ${prefix}_${file_name}_cleanup
        COMMAND ${BASH} -c "rm -rf ${working_directory}/tmp* ${working_directory}/*-tmp-*"
        WORKING_DIRECTORY ${working_directory}
    )
    set_tests_properties(${prefix}_${file_name}_cleanup PROPERTIES FIXTURES_SETUP ${file_name}_cleanup)

    file(STRINGS ${file_path} FILE_CONTENTS)
    list(LENGTH FILE_CONTENTS FILE_CONTENTS_LENGTH)
    math(EXPR MAX_LINE_NUM "${FILE_CONTENTS_LENGTH} - 1")


    set(EXEC_DIR_PATHS "${exec_directories}")
    string(REPLACE ";" ":" EXEC_DIR_PATHS "${EXEC_DIR_PATHS}")

    foreach(line_num RANGE ${MAX_LINE_NUM})
        list(GET FILE_CONTENTS ${line_num} line)

        set(test_name ${file_name}) 
        if(${FILE_CONTENTS_LENGTH} GREATER 1)
            math(EXPR suffix "${line_num} + 1")
            set(test_name "${file_name}_${suffix}")
        endif()

        add_test(
            NAME ${prefix}_${test_name}
            COMMAND ${BASH} -c "export PATH=${EXEC_DIR_PATHS}:$PATH;${line}"
            WORKING_DIRECTORY ${working_directory}
        )
        set_tests_properties(${prefix}_${test_name} 
            PROPERTIES FIXTURES_REQUIRED ${file_name}_cleanup
        )
        message(VERBOSE "Add bash tests commands for ${file_name}: ${line}")
    endforeach()
endfunction()