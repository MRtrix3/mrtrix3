include_guard(GLOBAL)

# Parse the "MRTRIX_DEPENDENCIES" constant of a single MRtrix3 Python command.
#
# The MRtrix3 commands that a given Python command may invoke at execution are declared
#   within that command's source code as a module-level list-of-strings constant named
#   "MRTRIX_DEPENDENCIES". This function extracts that list by direct textual parsing of the
#   source file, without invoking the Python interpreter (which is unavailable as a build
#   dependency at configure time, and which we explicitly wish to avoid executing).
#
# Arguments:
#   CMDNAME          Name of the Python command to interrogate.
#   COMMANDS_DIR     Filesystem path to the directory containing the Python commands
#                      (i.e. "python/mrtrix3/commands").
#   OUT_DEPENDENCIES Name of a variable, set in the caller's scope, that receives the list of
#                      directly-declared MRtrix3 command dependencies (empty if the command
#                      declares no dependencies, or if the constant could not be located).
#   OUT_DEFINED      Name of a variable, set in the caller's scope, to TRUE if the
#                      "MRTRIX_DEPENDENCIES" constant was successfully located, or FALSE if the
#                      command fails to define it.
#
# A Python command's source may be either a single file "<CMDNAME>.py" or a package directory
#   "<CMDNAME>/__init__.py"; both layouts are supported.
function(mrtrix_python_command_direct_dependencies CMDNAME COMMANDS_DIR OUT_DEPENDENCIES OUT_DEFINED)
    set(SINGLE_FILE "${COMMANDS_DIR}/${CMDNAME}.py")
    set(PACKAGE_FILE "${COMMANDS_DIR}/${CMDNAME}/__init__.py")
    if(EXISTS "${SINGLE_FILE}")
        set(SOURCE_FILE "${SINGLE_FILE}")
    elseif(EXISTS "${PACKAGE_FILE}")
        set(SOURCE_FILE "${PACKAGE_FILE}")
    else()
        message(FATAL_ERROR
            "Unable to locate source file for MRtrix3 Python command \"${CMDNAME}\" "
            "(looked for \"${SINGLE_FILE}\" and \"${PACKAGE_FILE}\")")
    endif()

    file(READ "${SOURCE_FILE}" SOURCE_CONTENTS)

    # Locate the assignment and capture the contents enclosed by the square brackets.
    # The negated character class "[^]]" matches newlines, permitting the list to span
    #   multiple lines in the source.
    if(NOT SOURCE_CONTENTS MATCHES "MRTRIX_DEPENDENCIES[ \t]*=[ \t]*\\[([^]]*)\\]")
        message(WARNING
            "MRtrix3 Python command \"${CMDNAME}\" does not define the MRTRIX_DEPENDENCIES constant; "
            "all MRtrix3 commands will be treated as compilation dependencies for safety")
        set(${OUT_DEPENDENCIES} "" PARENT_SCOPE)
        set(${OUT_DEFINED} FALSE PARENT_SCOPE)
        return()
    endif()

    set(LIST_BODY "${CMAKE_MATCH_1}")

    # Extract each quoted token (single- or double-quoted) as an individual command name.
    set(DEPENDENCIES "")
    string(REGEX MATCHALL "['\"][A-Za-z0-9_]+['\"]" QUOTED_TOKENS "${LIST_BODY}")
    foreach(QUOTED_TOKEN IN LISTS QUOTED_TOKENS)
        string(REGEX REPLACE "^['\"](.*)['\"]$" "\\1" COMMAND_NAME "${QUOTED_TOKEN}")
        list(APPEND DEPENDENCIES "${COMMAND_NAME}")
    endforeach()

    if(DEPENDENCIES)
        list(REMOVE_DUPLICATES DEPENDENCIES)
        list(SORT DEPENDENCIES)
    endif()

    set(${OUT_DEPENDENCIES} "${DEPENDENCIES}" PARENT_SCOPE)
    set(${OUT_DEFINED} TRUE PARENT_SCOPE)
endfunction()
