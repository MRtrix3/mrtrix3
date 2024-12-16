function(mrtrix_print_build_instructions)
    set(message_list "\n")
    list(APPEND message_list "MRtrix3 has been successfully configured for compilation.")
    list(APPEND message_list "To build MRtrix3, run 'cmake --build .' from the build directory.")

    list(APPEND message_list "To reconfigure the project, run CMake again with the same arguments.")
    list(APPEND message_list "If reconfiguration fails, remove the CMakeCache.txt file in the build "
        "directory or run CMake adding the --fresh flag.")
    list(APPEND message_list "\n")

    # Remove ; from list and add two spaces to the beginning of each line
    string(REPLACE ";" "\n  " message_list "${message_list}")
    message(STATUS "${message_list}")
endfunction()
