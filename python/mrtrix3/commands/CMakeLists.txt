
set(PYTHON_COMMANDS_INIT_FILE ${PROJECT_BINARY_DIR}/lib/mrtrix3/commands/__init__.py)

file(GLOB PYTHON_ALL_COMMANDS_ROOT_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/*
)

file(GLOB_RECURSE PYTHON_ALL_COMMANDS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.py
)

set(PYTHON_COMMAND_LIST "")
foreach(PYTHON_PATH ${PYTHON_ALL_COMMANDS_ROOT_PATHS})
    get_filename_component(CMDNAME ${PYTHON_PATH} NAME_WE)
    if(NOT ${CMDNAME} STREQUAL "CMakeLists" AND NOT ${CMDNAME} STREQUAL "__init__")
        list(APPEND PYTHON_COMMAND_LIST ${CMDNAME})
    endif()
endforeach()

if(MRTRIX_PYTHON_SOFTLINK)
    set(PYTHON_COMMANDS_TARGET_NAME "LinkPythonCommandFiles")
    set(PYTHON_API_TARGET_NAME "LinkPythonCommandFiles")
    set(PYTHON_COMMANDS_FUNCTION_NAME "create_symlink")
else()
    set(PYTHON_COMMANDS_TARGET_NAME "CopyPythonCommandFiles")
    set(PYTHON_API_TARGET_NAME "CopyPythonCommandFiles")
    set(PYTHON_COMMANDS_FUNCTION_NAME "copy_if_different")
endif()

add_custom_target(${PYTHON_COMMANDS_TARGET_NAME} ALL)
add_dependencies(${PYTHON_COMMANDS_TARGET_NAME} ${PYTHON_API_TARGET_NAME})

set(PYTHON_BUILD_COMMAND_FILES "")

# Have to append commands to create all directories
#   before commands to symlink files can appear
# Use presence of "__init__.py" as proxy for the need to construct a directory
foreach(PYTHON_SRC_PATH ${PYTHON_ALL_COMMANDS_FILES})
    get_filename_component(FILENAME ${PYTHON_SRC_PATH} NAME)
    if(${FILENAME} STREQUAL "__init__.py")
        file(RELATIVE_PATH REL_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_SRC_PATH})
        get_filename_component(DIRNAME ${REL_PATH} DIRECTORY)
        add_custom_command(
            TARGET ${PYTHON_COMMANDS_TARGET_NAME}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/lib/mrtrix3/commands/${DIRNAME}
        )
    endif()
endforeach()

foreach(PYTHON_SRC_PATH ${PYTHON_ALL_COMMANDS_FILES})
    file(RELATIVE_PATH DST_RELPATH ${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_SRC_PATH})
    # Skip "commands/__init__.py";
    #   this file will be written separately via execution of "commands/__init__.py.in"
    if (${DST_RELPATH} STREQUAL "__init__.py")
        continue()
    endif()
    set(DST_BUILDPATH ${PROJECT_BINARY_DIR}/lib/mrtrix3/commands/${DST_RELPATH})
    add_custom_command(
        TARGET ${PYTHON_COMMANDS_TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E ${PYTHON_COMMANDS_FUNCTION_NAME} ${PYTHON_SRC_PATH} ${DST_BUILDPATH}
        DEPENDS ${PYTHON_SRC_PATH}
    )
    get_filename_component(DST_INSTALLDIR ${DST_RELPATH} DIRECTORY)
    install(FILES ${PYTHON_SRC_PATH}
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/mrtrix3/commands/${DST_INSTALLDIR}
    )
    list(APPEND PYTHON_BUILD_COMMAND_FILES ${DST_PATH})
endforeach()

add_custom_target(MakePythonExecutables ALL)

set(PYTHON_BIN_FILES "")
foreach(CMDNAME ${PYTHON_COMMAND_LIST})
    add_custom_command(
        TARGET MakePythonExecutables
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -DCMDNAME=${CMDNAME} -DBUILDDIR=${PROJECT_BINARY_DIR} -P ${PROJECT_SOURCE_DIR}/cmake/MakePythonExecutable.cmake
    )
    list(APPEND PYTHON_BIN_FILES ${PROJECT_BINARY_DIR}/bin/${CMDNAME})
endforeach()


# We need to generate a list of MRtrix3 commands:
#   function run.command() does different things if it is executing an MRtrix3 command vs. an external command,
#   but unlike prior software versions we cannot simply interrogate the contents of the bin/ directory at runtime
add_custom_target(MakePythonCommandsInit ALL)
add_dependencies(MakePythonCommandsInit ${PYTHON_API_TARGET_NAME})
add_custom_command(
    TARGET MakePythonCommandsInit
    COMMAND ${CMAKE_COMMAND}
    -D DST=${PYTHON_COMMANDS_INIT_FILE}
    -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/__init__.py.in
    -P ${PROJECT_SOURCE_DIR}/cmake/GenPythonCommandsLists.cmake
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

install(FILES ${PYTHON_BIN_FILES}
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${PYTHON_COMMANDS_INIT_FILE}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/mrtrix3/commands/
)
