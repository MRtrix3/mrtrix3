set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

if(EXISTS ${ROOT_DIR}/.git AND PROJECT_IS_TOP_LEVEL)
    if(GIT_EXECUTABLE)
        message(VERBOSE "Git found: ${GIT_EXECUTABLE}")
        # Get tag
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --abbrev=0
            WORKING_DIRECTORY ${ROOT_DIR}
            OUTPUT_VARIABLE MRTRIX_GIT_TAG
            RESULT_VARIABLE MRTRIX_GIT_TAG_ERROR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        # Get commit
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --abbrev=8 --dirty --always
            WORKING_DIRECTORY ${ROOT_DIR}
            OUTPUT_VARIABLE MRTRIX_GIT_COMMIT
            RESULT_VARIABLE MRTRIX_GIT_COMMIT_ERROR
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(NOT MRTRIX_GIT_TAG_ERROR AND NOT MRTRIX_GIT_COMMIT_ERROR)
            message(VERBOSE "Git tag: ${MRTRIX_GIT_TAG}; Git commit: ${MRTRIX_GIT_COMMIT}")
            if(NOT "${MRTRIX_GIT_TAG}" STREQUAL ${MRTRIX_BASE_VERSION})
                message(FATAL_ERROR "MRtrix3 base version does not match the git tag!")
            endif()
            set(MRTRIX_VERSION ${MRTRIX_GIT_COMMIT})
        else()
            message(WARNING "Failed to get git tag and commit. Version will be set to default base version.")
        endif()
    else()
        message(WARNING "Git not found on this system. Version will be set to default base version.")
    endif()
endif()

if(NOT MRTRIX_VERSION)
    set(MRTRIX_VERSION ${MRTRIX_BASE_VERSION})
endif()


configure_file(
    ${SRC}
    ${DST}
    @ONLY
)
