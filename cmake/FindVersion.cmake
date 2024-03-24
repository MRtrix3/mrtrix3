if(GIT_EXECUTABLE)
    message(VERBOSE "Git found: ${GIT_EXECUTABLE}")
    # Get tag
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --abbrev=0
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE MRTRIX_GIT_TAG
        RESULT_VARIABLE MRTRIX_GIT_TAG_ERROR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    # Get commit
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --abbrev=8 --dirty --always
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE MRTRIX_GIT_COMMIT
        RESULT_VARIABLE MRTRIX_GIT_COMMIT_ERROR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT "${MRTRIX_GIT_TAG}" STREQUAL ${MRTRIX_BASE_VERSION})
        message(FATAL_ERROR "MRtrix3 base version does not match the git tag!")
    endif()

    if(NOT MRTRIX_GIT_TAG_ERROR AND NOT MRTRIX_GIT_COMMIT_ERROR)
        message(VERBOSE "Git tag: ${MRTRIX_GIT_TAG}; Git commit: ${MRTRIX_GIT_COMMIT}")
        set(MRTRIX_VERSION ${MRTRIX_GIT_COMMIT})
    else()
        message(VERBOSE "Git tag: not found.")
    endif()
endif()

if(NOT MRTRIX_VERSION)
    set(MRTRIX_VERSION ${MRTRIX_BASE_VERSION})
    message(STATUS "Failed to determine version from Git, using default base version: ${MRTRIX_BASE_VERSION}")
endif()

configure_file(
    ${SRC}
    ${DST}
    @ONLY
)
