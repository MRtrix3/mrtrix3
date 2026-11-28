include_guard(GLOBAL)

# Parse the "MRTRIX_DEPENDENCIES" declarations within a single MRtrix3 Python source file.
#
# The MRtrix3 commands that a given Python command may invoke at execution are declared within its
#   source code as module-level assignments to a set-valued constant named "MRTRIX_DEPENDENCIES".
#   For a command whose source is spread across multiple files, each file contributes the MRtrix3
#   commands that it itself invokes: the package "__init__.py" seeds the constant ("= set()") and
#   every other file augments it ("|= {...}"). This function extracts the command names declared
#   within one such file by direct textual parsing, without invoking the Python interpreter (which
#   is unavailable as a build dependency at configure time, and which we explicitly wish to avoid
#   executing).
#
# Both the seeding ("=") and augmenting ("|=") forms are recognised, as are the collection spellings
#   "{...}" (set literal), "set([...])" and the empty "set()".
#
# Arguments:
#   SOURCE_FILE      Filesystem path to the Python source file to parse.
#   OUT_DEPENDENCIES Name of a variable, set in the caller's scope, that receives the list of MRtrix3
#                      command names declared within this file (empty if the file declares none).
#   OUT_DEFINED      Name of a variable, set in the caller's scope, to TRUE if the file contains at
#                      least one "MRTRIX_DEPENDENCIES" assignment ("=" or "|="), or FALSE otherwise.
function(mrtrix_python_file_dependencies SOURCE_FILE OUT_DEPENDENCIES OUT_DEFINED)
    file(READ "${SOURCE_FILE}" SOURCE_CONTENTS)

    # Capture every "MRTRIX_DEPENDENCIES" assignment (plain "=" or augmented "|=") together with the
    #   collection that follows it. Each capture runs from the constant name up to and including the
    #   first closing "}" or ")"; as command names never contain those characters, this reliably
    #   delimits the collection even when it spans multiple lines (the negated class "[^})]" matches
    #   newlines). "\\|?=" matches an optional literal pipe followed by "=", i.e. both "=" and "|=".
    string(REGEX MATCHALL
        "MRTRIX_DEPENDENCIES[ \t]*\\|?=[^})]*[})]"
        ASSIGNMENTS "${SOURCE_CONTENTS}")

    if(NOT ASSIGNMENTS)
        set(${OUT_DEPENDENCIES} "" PARENT_SCOPE)
        set(${OUT_DEFINED} FALSE PARENT_SCOPE)
        return()
    endif()

    # Extract each quoted token (single- or double-quoted) from across all assignments as an
    #   individual command name; an empty "set()" contributes no tokens.
    set(DEPENDENCIES "")
    foreach(ASSIGNMENT IN LISTS ASSIGNMENTS)
        string(REGEX MATCHALL "['\"][A-Za-z0-9_]+['\"]" QUOTED_TOKENS "${ASSIGNMENT}")
        foreach(QUOTED_TOKEN IN LISTS QUOTED_TOKENS)
            string(REGEX REPLACE "^['\"](.*)['\"]$" "\\1" COMMAND_NAME "${QUOTED_TOKEN}")
            list(APPEND DEPENDENCIES "${COMMAND_NAME}")
        endforeach()
    endforeach()

    if(DEPENDENCIES)
        list(REMOVE_DUPLICATES DEPENDENCIES)
        list(SORT DEPENDENCIES)
    endif()

    set(${OUT_DEPENDENCIES} "${DEPENDENCIES}" PARENT_SCOPE)
    set(${OUT_DEFINED} TRUE PARENT_SCOPE)
endfunction()
