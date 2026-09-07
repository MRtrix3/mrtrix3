# A function that adds a bash test for an input bash script file
function(add_bash_test)
    find_program(BASH bash)

    if(NOT BASH)
        message(FATAL_ERROR "bash not found")
    endif()

    set(options NO_FILESYSTEM_LOCK)
    set(singleValueArgs FILE_PATH PREFIX WORKING_DIRECTORY ENVIRONMENT)
    set(multiValueArgs EXEC_DIRECTORIES LABELS)
    cmake_parse_arguments(
        ARG
        "${options}"
        "${singleValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    set(file_path ${ARG_FILE_PATH})
    set(prefix ${ARG_PREFIX})
    set(working_directory ${ARG_WORKING_DIRECTORY})
    set(exec_directories ${ARG_EXEC_DIRECTORIES})
    set(environment ${ARG_ENVIRONMENT})
    set(labels ${ARG_LABELS})

    # Regenerate tests when the test script changes
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${file_path})

    get_filename_component(file_name ${file_path} NAME_WE)
    set(test_name ${prefix}_${file_name})

    # Add a custom target for IDEs to pickup the test script
    add_custom_target(test_${prefix}_${file_name} SOURCES ${file_path})

    string(REPLACE ";" ":" exec_directories "${exec_directories}")

    # Bash tests sharing a working directory write, overwrite and delete temporary
    # filesystem content prefixed "tmp"; running two such tests concurrently lets
    # one test's cleanup clobber another's in-flight temporaries.
    # Tests are serialised per working directory by two complementary mechanisms:
    #   - the CTest RESOURCE_LOCK property (within a single ctest invocation), and
    #   - a filesystem-level lock in RunTest.cmake (across invocations/instances).
    # A test that the developer certifies never writes to the filesystem can opt
    # out of both via NO_FILESYSTEM_LOCK, leaving it free to run in parallel.
    set(run_test_args
        -D BASH=${BASH}
        -D FILE_PATH=${file_path}
        -D WORKING_DIRECTORY=${working_directory}
    )
    if(NOT ARG_NO_FILESYSTEM_LOCK)
        # The lock file lives inside the guarded directory itself, so every test
        # sharing that directory (including across build trees that share it)
        # agrees on the same lock; its fixed name avoids the "tmp" cleanup glob.
        set(cleanup_cmd "rm -rf ${working_directory}/tmp* ${working_directory}/*-tmp-*")
        set(lock_file "${working_directory}/.mrtrix_testing.lock")
        list(APPEND run_test_args
            -D CLEANUP_CMD=${cleanup_cmd}
            -D LOCKFILE=${lock_file}
        )
    endif()

    add_test(
        NAME ${test_name}
        COMMAND
            ${CMAKE_COMMAND}
            ${run_test_args}
            -P ${PROJECT_SOURCE_DIR}/cmake/RunTest.cmake
    )
    set_tests_properties(${test_name}
        PROPERTIES
        ENVIRONMENT "PATH=${exec_directories};${environment}"
    )
    if(NOT ARG_NO_FILESYSTEM_LOCK)
        # One resource per unique working directory: tests holding the same lock
        # string are never scheduled concurrently by a single ctest invocation.
        set_tests_properties(${test_name}
            PROPERTIES
            RESOURCE_LOCK "${working_directory}"
        )
    endif()
    if(labels)
        set_tests_properties(${test_name} PROPERTIES LABELS "${labels}")
    endif()

    message(VERBOSE "Added bash test command ${test_name}")
endfunction()
