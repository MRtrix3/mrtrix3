set(PYTHON_VERSION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/lib/mrtrix3/_version.py)

find_package(Git QUIET)

file(GLOB_RECURSE PYTHON_BIN_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/*
)

file(GLOB_RECURSE PYTHON_LIB_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/*
)

add_custom_target(Python SOURCES
    ${PYTHON_BIN_FILES}
)

# We generate the version file at configure time,
# so tools like Pylint can run without building the project
execute_process(
    COMMAND ${CMAKE_COMMAND} 
    -D GIT_EXECUTABLE=${GIT_EXECUTABLE}
    -D MRTRIX_BASE_VERSION=${MRTRIX_BASE_VERSION}
    -D DST=${PYTHON_VERSION_FILE}
    -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/lib/mrtrix3/_version.py.in
    -P ${PROJECT_SOURCE_DIR}/cmake/FindVersion.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(CopyPythonFiles ALL)
set(PYTHON_BUILD_BIN_FILES "")

foreach(BIN_FILE ${PYTHON_BIN_FILES})
    get_filename_component(BIN_FILE_NAME ${BIN_FILE} NAME)
    set(DST_BIN_FILE ${PROJECT_BINARY_DIR}/bin/${BIN_FILE_NAME})
    add_custom_command(
        TARGET CopyPythonFiles
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${BIN_FILE} ${DST_BIN_FILE}
        DEPENDS ${BIN_FILE}
    )
    list(APPEND PYTHON_BUILD_BIN_FILES ${DST_BIN_FILE})
endforeach()

add_custom_command(
    TARGET CopyPythonFiles
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/lib" "${PROJECT_BINARY_DIR}/lib"
)

set_target_properties(CopyPythonFiles 
    PROPERTIES ADDITIONAL_CLEAN_FILES
        "${PYTHON_BUILD_BIN_FILES};${PROJECT_BINARY_DIR}/lib"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    USE_SOURCE_PERMISSIONS
    PATTERN "__pycache__" EXCLUDE
    PATTERN ".pyc" EXCLUDE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    USE_SOURCE_PERMISSIONS
    PATTERN "__pycache__" EXCLUDE
    PATTERN "*.py.in" EXCLUDE
    PATTERN ".pyc" EXCLUDE
)

install(FILES ${PYTHON_VERSION_FILE}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/mrtrix3
)
