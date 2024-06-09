add_subdirectory(commands)

set(PYTHON_VERSION_FILE ${PROJECT_BINARY_DIR}/lib/mrtrix3/version.py)

find_package(Git QUIET)

file(GLOB PYTHON_LIB_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.py
)

if(MRTRIX_PYTHON_SOFTLINK)
    set(PYTHON_API_TARGET_NAME "LinkPythonAPIFiles")
    set(PYTHON_API_FUNCTION_NAME "create_symlink")
else()
    set(PYTHON_API_TARGET_NAME "CopyPythonAPIFiles")
    set(PYTHON_API_FUNCTION_NAME "copy_if_different")
endif()

add_custom_target(${PYTHON_API_TARGET_NAME} ALL)
add_custom_command(
    TARGET ${PYTHON_API_TARGET_NAME}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/lib/mrtrix3
)

set(PYTHON_BUILD_FILES "")
foreach(PYTHON_LIB_FILE ${PYTHON_LIB_FILES})
    get_filename_component(LIB_FILE_NAME ${PYTHON_LIB_FILE} NAME)
    # File "version.py" gets generated in the build directory by cmake based on "version.py.in";
    #   this file is only present for pylint, and should be skipped during build
    if (${LIB_FILE_NAME} STREQUAL "version.py")
        continue()
    endif()
    set(DST_LIB_FILE ${PROJECT_BINARY_DIR}/lib/mrtrix3/${LIB_FILE_NAME})
    add_custom_command(
        TARGET ${PYTHON_API_TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E ${PYTHON_API_FUNCTION_NAME} ${PYTHON_LIB_FILE} ${DST_LIB_FILE}
        DEPENDS ${LIB_FILE}
    )
    list(APPEND PYTHON_BUILD_FILES ${DST_LIB_FILE})
endforeach()

set_target_properties(${PYTHON_API_TARGET_NAME}
    PROPERTIES ADDITIONAL_CLEAN_FILES
        "${PYTHON_BUILD_FILES};${PROJECT_BINARY_DIR}/lib"
)

add_custom_target(MakePythonVersionFile ALL)
add_dependencies(MakePythonVersionFile ${PYTHON_API_TARGET_NAME})
add_custom_command(
    TARGET MakePythonVersionFile
    COMMAND ${CMAKE_COMMAND}
        -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
        -D MRTRIX_BASE_VERSION=${MRTRIX_BASE_VERSION}
        -D DST=${PYTHON_VERSION_FILE}
        -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/version.py.in
        -P ${PROJECT_SOURCE_DIR}/cmake/FindVersion.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_target_properties(MakePythonVersionFile
    PROPERTIES ADDITIONAL_CLEAN_FILES ${PYTHON_VERSION_FILE}
)

install(FILES ${PYTHON_LIB_FILES}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/mrtrix3
)

install(FILES ${PYTHON_VERSION_FILE}
    PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/mrtrix3
)
