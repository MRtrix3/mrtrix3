# A function that adds a bash test for an input bash script file
function(add_bash_test)
    find_program(BASH bash)

    if(NOT BASH)
        message(FATAL_ERROR "bash not found")
    endif()

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

    get_filename_component(file_name ${file_path} NAME_WE)
    set(test_name ${prefix}_${file_name})

    # Add a custom target for IDEs to pickup the test script
    add_custom_target(test_${prefix}_${file_name} SOURCES ${file_path})

    # Ensure that "rm" and its dependencies will be included in PATH
    list(APPEND exec_directories "$ENV{PATH}")

    # In MINGW, we need to replace paths starting with drive:/ with /drive/
    # when invoking a bash command (e.g. using bash -c "command")
    if(MINGW AND WIN32)
        foreach(exec_dir ${EXEC_DIR_PATHS})
            EXECUTE_PROCESS(
                COMMAND cygpath -u ${exec_dir}
                OUTPUT_VARIABLE new_exec_dir
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            list(APPEND EXEC_DIR_PATHS ${new_exec_dir})
        endforeach()
    else()
        set(EXEC_DIR_PATHS "${exec_directories}")
    endif()

    string(REPLACE ";" ":" EXEC_DIR_PATHS "${EXEC_DIR_PATHS}")

    set(cleanup_cmd "rm -rf ${working_directory}/tmp* ${working_directory}/*-tmp-*")

    add_test(
        NAME ${test_name}
        COMMAND
            ${CMAKE_COMMAND}
            -D BASH=${BASH}
            -D FILE_PATH=${file_path}
            -D CLEANUP_CMD=${cleanup_cmd}
            -D WORKING_DIRECTORY=${working_directory}
            -P ${PROJECT_SOURCE_DIR}/cmake/RunTest.cmake
    )
    set_tests_properties(${test_name}
        PROPERTIES
        ENVIRONMENT "PATH=${EXEC_DIR_PATHS}"
    )
    if(labels)
        set_tests_properties(${test_name} PROPERTIES LABELS "${labels}")
    endif()

    message(VERBOSE "Add bash tests commands for ${test_name}: ${line}")
endfunction()
