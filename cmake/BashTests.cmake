# A function that adds a bash test for each line in a given file
function(add_bash_tests)
    set(singleValueArgs FILE_PATH PREFIX WORKING_DIRECTORY)
    set(multiValueArgs EXEC_DIRECTORIES LABELS)
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
    set(labels ${ARG_LABELS})

    # Regenerate tests when the test script changes
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${file_path})

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
    set(test_name ${prefix}_${file_name})
    
    # Add a custom target for IDEs to pickup the test script
    add_custom_target(test_${prefix}_${file_name} SOURCES ${file_path})

    # Add test that cleans up temporary files
    add_test(
        NAME ${test_name}_cleanup
        COMMAND ${BASH} -c "rm -rf ${working_directory}/tmp* ${working_directory}/*-tmp-*"
        WORKING_DIRECTORY ${working_directory}
    )
    set_tests_properties(${test_name}_cleanup PROPERTIES FIXTURES_SETUP ${test_name}_cleanup)

    file(STRINGS ${file_path} FILE_CONTENTS)
    list(LENGTH FILE_CONTENTS FILE_CONTENTS_LENGTH)
    math(EXPR MAX_LINE_NUM "${FILE_CONTENTS_LENGTH} - 1")


    set(EXEC_DIR_PATHS "${exec_directories}")
    string(REPLACE ";" ":" EXEC_DIR_PATHS "${EXEC_DIR_PATHS}")

    add_test(
	NAME ${test_name}
	COMMAND ${BASH} "${file_path}"
        WORKING_DIRECTORY ${working_directory}
    )
    set_tests_properties(${test_name}
        PROPERTIES 
	    ENVIRONMENT "PATH=${EXEC_DIR_PATHS}:$ENV{PATH}"
	    FIXTURES_REQUIRED ${test_name}_cleanup
    )
    if(labels)
	message(STATUS "Setting labels for ${prefix}_${test_name}: ${labels}")
	set_tests_properties(${test_name} PROPERTIES LABELS "${labels}")
	set_tests_properties(${test_name}_cleanup PROPERTIES LABELS "${labels}")
    endif()

    message(VERBOSE "Add bash tests commands for ${test_name}: ${line}")
endfunction()
