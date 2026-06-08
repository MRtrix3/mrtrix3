include_guard(GLOBAL)

include(${CMAKE_CURRENT_LIST_DIR}/ParsePythonCommandDependencies.cmake)

# Recursively resolve the complete set of MRtrix3 commands that must be compiled in order for a
#   given MRtrix3 Python command to be able to execute.
#
# A Python command declares (via its "MRTRIX_DEPENDENCIES" constant; see
#   ParsePythonCommandDependencies.cmake) the MRtrix3 commands that it may invoke directly. Those
#   dependencies may themselves be Python commands, which in turn carry their own dependencies;
#   this function follows such Python-to-Python invocations transitively so that the returned set
#   is closed under the dependency relation.
#
# If the top-level command, or any Python command reached during traversal, fails to declare its
#   MRTRIX_DEPENDENCIES, then the dependency set cannot be known. In that case the only safe
#   outcome is to require that every MRtrix3 command be built; the function therefore returns the
#   comprehensive set of all MRtrix3 commands (both C++ and Python).
#
# Arguments:
#   CMDNAME              Name of the top-level Python command to resolve.
#   COMMANDS_DIR         Filesystem path to the directory containing the Python commands.
#   ALL_PYTHON_COMMANDS  Complete list of all MRtrix3 Python command names (used to distinguish
#                          Python-command dependencies, which are recursed into, from C++ ones).
#   ALL_COMMANDS         Complete list of all MRtrix3 command names (C++ and Python); returned as
#                          the safe fallback when any required dependency list is undeclared.
#   OUT_DEPENDENCIES     Name of a variable, set in the caller's scope, that receives the resolved,
#                          de-duplicated, sorted set of MRtrix3 commands that must be built. The
#                          command itself (CMDNAME) is excluded from this set.
function(mrtrix_python_command_build_dependencies
         CMDNAME COMMANDS_DIR ALL_PYTHON_COMMANDS ALL_COMMANDS OUT_DEPENDENCIES)

    set(RESOLVED "")           # Commands confirmed as build dependencies
    set(VISITED_PYTHON "")     # Python commands whose dependencies have been (or are being) expanded
    set(PENDING "${CMDNAME}")  # Worklist of Python commands awaiting expansion
    set(FALLBACK FALSE)

    while(PENDING)
        list(POP_FRONT PENDING CURRENT)

        # Avoid re-expanding a Python command (and thereby guard against dependency cycles,
        #   e.g. dwi2mask <-> dwi2response).
        list(FIND VISITED_PYTHON "${CURRENT}" CURRENT_VISITED_INDEX)
        if(NOT CURRENT_VISITED_INDEX EQUAL -1)
            continue()
        endif()
        list(APPEND VISITED_PYTHON "${CURRENT}")

        mrtrix_python_command_direct_dependencies(
            "${CURRENT}" "${COMMANDS_DIR}" CURRENT_DEPS CURRENT_DEFINED)

        if(NOT CURRENT_DEFINED)
            set(FALLBACK TRUE)
            break()
        endif()

        foreach(DEP IN LISTS CURRENT_DEPS)
            list(APPEND RESOLVED "${DEP}")
            # Recurse into Python-command dependencies so that their dependencies are captured too.
            list(FIND ALL_PYTHON_COMMANDS "${DEP}" DEP_PYTHON_INDEX)
            list(FIND VISITED_PYTHON "${DEP}" DEP_VISITED_INDEX)
            if((NOT DEP_PYTHON_INDEX EQUAL -1) AND (DEP_VISITED_INDEX EQUAL -1))
                list(APPEND PENDING "${DEP}")
            endif()
        endforeach()
    endwhile()

    if(FALLBACK)
        set(RESOLVED "${ALL_COMMANDS}")
    endif()

    # A command can never be a compilation dependency of itself.
    if(RESOLVED)
        list(REMOVE_ITEM RESOLVED "${CMDNAME}")
        list(REMOVE_DUPLICATES RESOLVED)
        list(SORT RESOLVED)
    endif()

    set(${OUT_DEPENDENCIES} "${RESOLVED}" PARENT_SCOPE)
endfunction()
