function(mrtrix_print_build_instructions)
    set(message_list "\n")
    file(RELATIVE_PATH relative_build_path "${PROJECT_SOURCE_DIR}" "${PROJECT_BINARY_DIR}")
    list(APPEND message_list "MRtrix3 has been successfully configured for compilation.")
    list(APPEND message_list "To build it, run 'cmake --build ${relative_build_path}'.")

    list(APPEND message_list "To reconfigure the project, run CMake again with the same arguments.")

    set(RECONFIGURE_ADVICE "If reconfiguration fails, try removing the 'CMakeCache.txt' file in the build directory first.")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.24)
        set(RECONFIGURE_ADVICE "${RECONFIGURE_ADVICE} Alternatively, you can reconfigure the project with the '--fresh' flag"
                               " (e.g. 'cmake -B build --fresh').")
    endif()

    list(APPEND message_list "${RECONFIGURE_ADVICE}")
    list(APPEND message_list "\n")

    # Remove ; from list and add two spaces to the beginning of each line
    string(REPLACE ";" "\n  " message_list "${message_list}")
    message(STATUS "${message_list}")
endfunction()
