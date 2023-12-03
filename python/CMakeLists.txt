set(PYTHON_VERSION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/_version.py)

set(CMAKE_INSTALL_SRCDIR ${CMAKE_INSTALL_LIBDIR}/../src/)

find_package(Git QUIET)

file(GLOB PYTHON_LIB_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.py
)

file(GLOB_RECURSE PYTHON_ALL_SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.py
)

file(GLOB PYTHON_ALL_COMMANDS
    ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*
)

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
add_custom_command(
    TARGET LinkPythonFiles
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/src/mrtrix3
)
foreach(SUBDIR ${PYTHON_ALL_COMMANDS})
    if(IS_DIRECTORY ${SUBDIR})
        get_filename_component(CMDNAME ${SUBDIR} NAME)
        set(MODULENAME ${CMDNAME})
        if(MODULENAME MATCHES "^[0-9].*$")
            set(MODULENAME _${MODULENAME})
        endif()
        add_custom_command(
            TARGET LinkPythonFiles
            COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/src/mrtrix3/${MODULENAME}
        )
    endif()
endforeach()
set(PYTHON_BUILD_LIB_FILES "")
set(PYTHON_BUILD_SRC_FILES "")
foreach(LIB_FILE ${PYTHON_LIB_FILES})
    get_filename_component(LIB_FILE_NAME ${LIB_FILE} NAME)
    set(DST_LIB_FILE ${PROJECT_BINARY_DIR}/lib/mrtrix3/${LIB_FILE_NAME})
    add_custom_command(
        TARGET LinkPythonFiles
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${LIB_FILE} ${DST_LIB_FILE}
        DEPENDS ${LIB_FILE}
    )
    list(APPEND PYTHON_BUILD_LIB_FILES ${DST_LIB_FILE})
endforeach()
foreach(SRC_FILE ${PYTHON_ALL_SRC_FILES})
    file(RELATIVE_PATH SRC_RELPATH ${CMAKE_CURRENT_SOURCE_DIR}/scripts ${SRC_FILE})
    if(SRC_RELPATH MATCHES "^[0-9].*$")
        set(SRC_RELPATH _${SRC_RELPATH})
    endif()
    set(DST_SRC_FILE ${PROJECT_BINARY_DIR}/src/mrtrix3/${SRC_RELPATH})
    add_custom_command(
        TARGET LinkPythonFiles
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${SRC_FILE} ${DST_SRC_FILE}
        DEPENDS ${SRC_FILE}
    )
    get_filename_component(SRC_RELPATH_DIRECTORY ${SRC_RELPATH} DIRECTORY)
    install(FILES ${SRC_FILE} DESTINATION ${CMAKE_INSTALL_SRCDIR}/mrtrix3/${SRC_RELPATH_DIRECTORY})
    list(APPEND PYTHON_BUILD_SRC_FILES ${DST_SRC_FILE})
endforeach()
set(PYTHON_BIN_FILES "")
foreach(CMDNAME ${PYTHON_ALL_COMMANDS})
    # Strip .py extension
    get_filename_component(BINNAME ${CMDNAME} NAME_WE)
    set(MODULENAME ${BINNAME})
    if(MODULENAME MATCHES "^[0-9].*$")
        set(MODULENAME _${MODULENAME})
    endif()
    if(CMDNAME MATCHES "^.*\.py$")
        set(SPECPATH "'${MODULENAME}.py'")
    else()
        set(SPECPATH "'${MODULENAME}', '__init__.py'")
    endif()
    set(BINPATH ${PROJECT_BINARY_DIR}/python/${BINNAME})
    file(WRITE ${BINPATH} "#!${Python3_EXECUTABLE}\n")
    file(APPEND ${BINPATH} "# -*- coding: utf-8 -*-\n")
    file(APPEND ${BINPATH} "import importlib.util\n")
    file(APPEND ${BINPATH} "import os\n")
    file(APPEND ${BINPATH} "import sys\n")
    file(APPEND ${BINPATH} "\n")
    file(APPEND ${BINPATH} "api_location = os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', 'lib', 'mrtrix3', '__init__.py'))\n")
    file(APPEND ${BINPATH} "api_spec = importlib.util.spec_from_file_location('mrtrix3', api_location)\n")
    file(APPEND ${BINPATH} "api_module = importlib.util.module_from_spec(api_spec)\n")
    file(APPEND ${BINPATH} "sys.modules['mrtrix3'] = api_module\n")
    file(APPEND ${BINPATH} "api_spec.loader.exec_module(api_module)\n")
    file(APPEND ${BINPATH} "\n")
    file(APPEND ${BINPATH} "src_spec = importlib.util.spec_from_file_location('${MODULENAME}',\n")
    file(APPEND ${BINPATH} "    os.path.normpath(os.path.join(os.path.dirname(os.path.realpath(__file__)), '..', 'src', 'mrtrix3', ${SPECPATH})))\n")
    file(APPEND ${BINPATH} "src_module = importlib.util.module_from_spec(src_spec)\n")
    file(APPEND ${BINPATH} "sys.modules[src_spec.name] = src_module\n")
    file(APPEND ${BINPATH} "src_spec.loader.exec_module(src_module)\n")
    file(APPEND ${BINPATH} "\n")
    file(APPEND ${BINPATH} "from mrtrix3.app import _execute\n")
    file(APPEND ${BINPATH} "import ${MODULENAME}\n")
    file(APPEND ${BINPATH} "_execute(${MODULENAME})\n")
    file(COPY ${BINPATH} DESTINATION ${PROJECT_BINARY_DIR}/bin
        FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ)
    list(APPEND PYTHON_BIN_FILES ${PROJECT_BINARY_DIR}/bin/${BINNAME})
endforeach()

set_target_properties(LinkPythonFiles
    PROPERTIES ADDITIONAL_CLEAN_FILES
        "${PYTHON_BUILD_LIB_FILES};${PROJECT_BINARY_DIR}/lib;${PYTHON_BUILD_SRC_FILES};${PROJECT_BINARY_DIR}/src;${PROJECT_BINARY_DIR}/python"
)

install(FILES ${PYTHON_BIN_FILES}
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_WRITE GROUP_READ WORLD_EXECUTE WORLD_READ
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${PYTHON_LIB_FILES}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/mrtrix3
)

install(FILES ${PYTHON_VERSION_FILE}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/mrtrix3
)
