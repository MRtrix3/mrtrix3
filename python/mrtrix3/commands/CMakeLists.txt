set(PYTHON_COMMANDS_INIT_FILE ${PROJECT_BINARY_DIR}/lib/mrtrix3/commands/__init__.py)

file(GLOB PYTHON_ALL_COMMANDS_ROOT_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/*"
)

file(GLOB_RECURSE PYTHON_ALL_COMMANDS_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.py"
)

# Collect a list of top-level command names (directories or single .py files).
set(PYTHON_COMMAND_LIST "")
foreach(PYTHON_PATH IN LISTS PYTHON_ALL_COMMANDS_ROOT_PATHS)
    get_filename_component(CMDNAME "${PYTHON_PATH}" NAME_WE)
    if(NOT "${CMDNAME}" STREQUAL "CMakeLists" AND NOT "${CMDNAME}" STREQUAL "__init__")
        list(APPEND PYTHON_COMMAND_LIST "${CMDNAME}")
    endif()
endforeach()

if(MRTRIX_PYTHON_SOFTLINK)
    set(PYTHON_API_TARGET_NAME "LinkPythonAPIFiles")
    set(PYTHON_COMMANDS_FUNCTION_NAME "create_symlink")
else()
    set(PYTHON_API_TARGET_NAME "CopyPythonAPIFiles")
    set(PYTHON_COMMANDS_FUNCTION_NAME "copy_if_different")
endif()


# Create commands folder in build directory prior to build phase
add_custom_target(MakePythonCommandsDir
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/lib/mrtrix3/commands"
)

set(ALL_PYTHON_COMMAND_TARGETS "")


# For each command:
# - create its own target
# - ensure corresponding subdirectories are created in build directory (if necessary)
# - copy/symlink Python files
# - create a stub Python launcher
foreach(CMDNAME IN LISTS PYTHON_COMMAND_LIST)
    file(GLOB_RECURSE CMD_PY_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/${CMDNAME}/*.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/${CMDNAME}.py"
    )

    set(CMD_TARGET "${CMDNAME}")
    add_custom_target(${CMD_TARGET})
    add_dependencies(${CMD_TARGET}
        ${PYTHON_API_TARGET_NAME}
        MakePythonCommandsDir
        MRtrixCppCommands
    )

    # For Python commands that need to be in a subdirectory, use presence of "__init__.py"
    # as proxy for the need to construct a directory
    foreach(PY_FILE IN LISTS CMD_PY_FILES)
        get_filename_component(FILENAME "${PY_FILE}" NAME)
        if("${FILENAME}" STREQUAL "__init__.py")
            file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${PY_FILE}")
            get_filename_component(DIRNAME "${REL_PATH}" DIRECTORY)
            add_custom_command(
                TARGET  ${CMD_TARGET}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                        "${PROJECT_BINARY_DIR}/lib/mrtrix3/commands/${DIRNAME}"
                COMMENT "Creating directory for Python command ${CMDNAME}"
            )
        endif()
    endforeach()

    # Copy/symlink Python files
    foreach(PY_FILE IN LISTS CMD_PY_FILES)
        file(RELATIVE_PATH DST_RELPATH "${CMAKE_CURRENT_SOURCE_DIR}" "${PY_FILE}")
        set(DST_BUILDPATH "${PROJECT_BINARY_DIR}/lib/mrtrix3/commands/${DST_RELPATH}")

        add_custom_command(
            TARGET  ${CMD_TARGET}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E ${PYTHON_COMMANDS_FUNCTION_NAME}
                    "${PY_FILE}" "${DST_BUILDPATH}"
            DEPENDS "${PY_FILE}"
            COMMENT "Copying Python script for ${CMDNAME}"
        )

        get_filename_component(DST_INSTALLDIR "${DST_RELPATH}" DIRECTORY)
        install(
            FILES "${PY_FILE}"
            PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/mrtrix3/commands/${DST_INSTALLDIR}"
        )
    endforeach()

    # Create stub Python launcher for the command
    add_custom_command(
        TARGET  ${CMD_TARGET}
        POST_BUILD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMAND ${CMAKE_COMMAND}
            -DCMDNAME=${CMDNAME}
            -DOUTPUT_DIR="${PROJECT_BINARY_DIR}/bin"
            -P "${PROJECT_SOURCE_DIR}/cmake/MakePythonExecutable.cmake"
        COMMENT "Creating Python stub for ${CMDNAME}"
    )

    install(
        FILES "${PROJECT_BINARY_DIR}/bin/${CMDNAME}"
        PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )

    list(APPEND ALL_PYTHON_COMMAND_TARGETS "${CMD_TARGET}")
endforeach()

add_custom_target(MRtrixPythonCommands ALL)
add_dependencies(MRtrixPythonCommands ${ALL_PYTHON_COMMAND_TARGETS})

# We need to generate a list of MRtrix3 commands:
# function run.command() does different things if it is executing an MRtrix3 command vs. an external command,
# but unlike prior software versions we cannot simply interrogate the contents of the bin/ directory at runtime
add_custom_target(MakePythonCommandsInit ALL)
add_dependencies(MakePythonCommandsInit MRtrixPythonCommands)
add_custom_command(
    TARGET MakePythonCommandsInit
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    -D DST=${PYTHON_COMMANDS_INIT_FILE}
    -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/__init__.py.in
    -P ${PROJECT_SOURCE_DIR}/cmake/GenPythonCommandsLists.cmake
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

install(
    FILES "${PYTHON_COMMANDS_INIT_FILE}"
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/mrtrix3/commands/"
)

add_custom_target(MRtrixPythonCommandsSources
    SOURCES ${PYTHON_ALL_COMMANDS_FILES}
)
