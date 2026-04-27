# A function that takes as input a list of directories,
#   and if cmake is being executed in a MinGW environment,
#   individually transform each directory to a Unix path using the "cygpath" command
# In MINGW, we need to replace paths starting with drive:/ with /drive/
#   when invoking a bash command (e.g. using bash -c "command")
function(dirs_to_unix output directories)
    if(MINGW AND WIN32)
        foreach(exec_dir ${directories})
            EXECUTE_PROCESS(
                COMMAND cygpath -u ${exec_dir}
                OUTPUT_VARIABLE new_exec_dir
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            list(APPEND result ${new_exec_dir})
        endforeach()
        set(${output} ${result} PARENT_SCOPE)
    else()
        set(${output} ${directories} PARENT_SCOPE)
    endif()
endfunction()
