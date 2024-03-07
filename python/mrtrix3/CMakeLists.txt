set(PYTHON_VERSION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/_version.py)

find_package(Git QUIET)

file(GLOB PYTHON_LIB_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.py
)

file(GLOB PYTHON_ROOT_ENTRIES
    ${CMAKE_CURRENT_SOURCE_DIR}/*
)

# TODO LIST_DIRECTORIES seems to not be having an effect here...
file(GLOB_RECURSE PYTHON_SRC_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/*/*
)

set(PYTHON_COMMAND_LIST "")
foreach(PYTHON_PATH ${PYTHON_ROOT_ENTRIES})
    if(IS_DIRECTORY ${PYTHON_PATH})
        get_filename_component(CMDNAME ${PYTHON_PATH} NAME)
        list(APPEND PYTHON_COMMAND_LIST ${CMDNAME})
    endif()
endforeach()

# We generate the version file at configure time,
# so tools like Pylint can run without building the project
execute_process(
    COMMAND ${CMAKE_COMMAND}
    -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
    -D MRTRIX_BASE_VERSION=${MRTRIX_BASE_VERSION}
    -D DST=${PYTHON_VERSION_FILE}
    -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/_version.py.in
    -P ${PROJECT_SOURCE_DIR}/cmake/FindVersion.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(LinkPythonFiles ALL)
add_custom_command(
    TARGET LinkPythonFiles
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/lib/mrtrix3
)

set(PYTHON_BUILD_FILES "")

foreach(PYTHON_LIB_FILE ${PYTHON_LIB_FILES})
    get_filename_component(LIB_FILE_NAME ${PYTHON_LIB_FILE} NAME)
    set(DST_LIB_FILE ${PROJECT_BINARY_DIR}/lib/mrtrix3/${LIB_FILE_NAME})
    add_custom_command(
        TARGET LinkPythonFiles
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${PYTHON_LIB_FILE} ${DST_LIB_FILE}
        DEPENDS ${LIB_FILE}
    )
    list(APPEND PYTHON_BUILD_FILES ${DST_LIB_FILE})
endforeach()

# Have to append commands to create all directories
#   before commands to symlink files can appear
set(PYTHON_DST_SUBDIRS "")
foreach(PYTHON_SRC_PATH ${PYTHON_SRC_PATHS})
    file(RELATIVE_PATH DST_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_SRC_PATH})
    # TODO Ideally would perform this check for every child directory
    if(DST_PATH MATCHES "^[0-9].*$")
        set(DST_PATH _${DST_PATH})
    endif()
    set(DST_PATH ${PROJECT_BINARY_DIR}/lib/mrtrix3/${DST_PATH})
    get_filename_component(FILENAME ${PYTHON_SRC_PATH} NAME)
    if(${FILENAME} STREQUAL "__init__.py")
        get_filename_component(DST_PARENTDIR ${DST_PATH} DIRECTORY)
        list(APPEND PYTHON_DST_SUBDIRS ${DST_PARENTDIR})
    endif()
endforeach()

foreach(PYTHON_DST_SUBDIR ${PYTHON_DST_SUBDIRS})
    add_custom_command(
        TARGET LinkPythonFiles
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PYTHON_DST_SUBDIR}
    )
endforeach()

foreach(PYTHON_SRC_PATH ${PYTHON_SRC_PATHS})
    file(RELATIVE_PATH DST_RELPATH ${CMAKE_CURRENT_SOURCE_DIR} ${PYTHON_SRC_PATH})
    # TODO Ideally would perform this check for every child directory
    if(DST_RELPATH MATCHES "^[0-9].*$")
        set(DST_RELPATH _${DST_RELPATH})
    endif()
    set(DST_BUILDPATH ${PROJECT_BINARY_DIR}/lib/mrtrix3/${DST_RELPATH})
    add_custom_command(
        TARGET LinkPythonFiles
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${PYTHON_SRC_PATH} ${DST_BUILDPATH}
        DEPENDS ${PYTHON_SRC_PATH}
    )
    set(DST_INSTALLPATH ${CMAKE_INSTALL_LIBDIR}/mrtrix3/${DST_RELPATH})
    install(FILES ${PYTHON_SRC_PATH} DESTINATION ${DST_INSTALLPATH})
    list(APPEND PYTHON_BUILD_FILES ${DST_PATH})
endforeach()

set(PYTHON_BIN_FILES "")
foreach(CMDNAME ${PYTHON_COMMAND_LIST})
    set(MODULENAME ${CMDNAME})
    if(MODULENAME MATCHES "^[0-9].*$")
        set(MODULENAME _${MODULENAME})
    endif()
    set(BINPATH "${PROJECT_BINARY_DIR}/temporary/python/${CMDNAME}")
    file(WRITE ${BINPATH} "#!${Python3_EXECUTABLE}\n")
    file(APPEND ${BINPATH} "# -*- coding: utf-8 -*-\n")
    file(APPEND ${BINPATH} "import os\n")
    file(APPEND ${BINPATH} "import sys\n")
    file(APPEND ${BINPATH} "\n")
    file(APPEND ${BINPATH} "sys.path.insert(0, os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), os.pardir, 'lib')))\n")
    file(APPEND ${BINPATH} "\n")
    file(APPEND ${BINPATH} "from mrtrix3.app import _execute\n")
    file(APPEND ${BINPATH} "from mrtrix3 import ${MODULENAME}\n")
    file(APPEND ${BINPATH} "_execute(${MODULENAME})\n")
    file(COPY ${BINPATH} DESTINATION ${PROJECT_BINARY_DIR}/bin
        FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ)
    list(APPEND PYTHON_BIN_FILES ${PROJECT_BINARY_DIR}/bin/${CMDNAME})
endforeach()

set_target_properties(LinkPythonFiles
    PROPERTIES ADDITIONAL_CLEAN_FILES
        "${PYTHON_BUILD_FILES};${PROJECT_BINARY_DIR}/lib;${PROJECT_BINARY_DIR}/temporary/python"
)

install(FILES ${PYTHON_BIN_FILES}
    PERMISSIONS OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${PYTHON_LIB_FILES}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/mrtrix3
)

install(FILES ${PYTHON_VERSION_FILE}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/mrtrix3
)
