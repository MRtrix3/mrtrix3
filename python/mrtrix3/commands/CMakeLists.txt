set(PYTHON_COMMANDS_INIT_FILE ${PROJECT_BINARY_DIR}/lib/mrtrix3/commands/__init__.py)

file(GLOB PYTHON_ALL_COMMANDS_ROOT_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/*"
)

file(GLOB_RECURSE PYTHON_ALL_COMMANDS_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.py"
)

# Collect a list of top-level command names: single "<name>.py" files, and package directories
#   that contain an "__init__.py". Selecting commands by this positive criterion (rather than
#   blocklisting known non-command entries) ensures that build-system artefacts deposited alongside
#   the sources by an in-source build -- e.g. the generated "CMakeFiles" directory,
#   "cmake_install.cmake", "CTestTestfile.cmake" -- are never mistaken for commands.
set(PYTHON_COMMAND_LIST "")
foreach(PYTHON_PATH IN LISTS PYTHON_ALL_COMMANDS_ROOT_PATHS)
    get_filename_component(FILENAME "${PYTHON_PATH}" NAME)
    if(IS_DIRECTORY "${PYTHON_PATH}")
        if(EXISTS "${PYTHON_PATH}/__init__.py")
            list(APPEND PYTHON_COMMAND_LIST "${FILENAME}")
        endif()
    elseif("${FILENAME}" MATCHES "\\.py$" AND NOT "${FILENAME}" STREQUAL "__init__.py")
        get_filename_component(CMDNAME "${PYTHON_PATH}" NAME_WE)
        list(APPEND PYTHON_COMMAND_LIST "${CMDNAME}")
    endif()
endforeach()

# Per-command compilation dependency resolution.
# Rather than requiring every C++ command to be built whenever any Python command is built,
#   each Python command declares (via its MRTRIX_DEPENDENCIES constant) the MRtrix3 commands that
#   it may invoke at execution; this is parsed and recursively resolved at configure time to
#   establish a minimal set of build targets per Python command (see below).
include(ResolvePythonCommandDependencies)

# Complete list of all MRtrix3 command names (C++ and Python), used as the safe fallback when a
#   command (or one of its transitive Python dependencies) fails to declare MRTRIX_DEPENDENCIES.
file(GLOB MRTRIX_CPP_COMMAND_SRCS "${PROJECT_SOURCE_DIR}/cpp/cmd/*.cpp")
set(MRTRIX_ALL_CPP_COMMANDS "")
foreach(CPP_SRC IN LISTS MRTRIX_CPP_COMMAND_SRCS)
    get_filename_component(CPP_CMDNAME "${CPP_SRC}" NAME_WE)
    list(APPEND MRTRIX_ALL_CPP_COMMANDS "${CPP_CMDNAME}")
endforeach()
set(MRTRIX_ALL_COMMANDS ${MRTRIX_ALL_CPP_COMMANDS} ${PYTHON_COMMAND_LIST})
list(REMOVE_DUPLICATES MRTRIX_ALL_COMMANDS)

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
    add_custom_target(${CMD_TARGET} DEPENDS ${CMD_PY_FILES})
    add_dependencies(${CMD_TARGET}
        ${PYTHON_API_TARGET_NAME}
        MakePythonCommandsDir
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

# Establish per-command build dependencies.
# The resolved dependency set already contains the full transitive closure of C++ commands reached
#   through any chain of Python-to-Python command invocations, so it is sufficient to add edges to
#   the C++ command targets only. Edges to other Python command targets are deliberately NOT added:
#   they are unnecessary (every Python command is built unconditionally via the MRtrixPythonCommands
#   target below), and mutually-recursive commands (e.g. dwi2mask <-> dwi2response) would otherwise
#   form a target dependency cycle, which CMake forbids among non-library targets.
# Only targets that actually exist are added; this gracefully skips both commands that are not built
#   in the current configuration (e.g. the GUI commands when MRTRIX_BUILD_GUI is disabled) and any
#   declared dependency that does not correspond to a build target.
foreach(CMDNAME IN LISTS PYTHON_COMMAND_LIST)
    mrtrix_python_command_build_dependencies(
        "${CMDNAME}"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PYTHON_COMMAND_LIST}"
        "${MRTRIX_ALL_COMMANDS}"
        CMD_BUILD_DEPENDENCIES)
    foreach(DEP IN LISTS CMD_BUILD_DEPENDENCIES)
        list(FIND PYTHON_COMMAND_LIST "${DEP}" DEP_IS_PYTHON_INDEX)
        if(TARGET ${DEP} AND DEP_IS_PYTHON_INDEX EQUAL -1)
            add_dependencies(${CMDNAME} ${DEP})
        endif()
    endforeach()
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
