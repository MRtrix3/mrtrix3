# This script can be invoked by calling ${CMAKE_COMMAND} -P <script_name>.cmake
# Runs a bash test described by the FILE_PATH variable
# The test is run in the WORKING_DIRECTORY directory
# The CLEANUP_CMD command, if provided, is run after the test is finished
# If LOCKFILE is provided, an exclusive filesystem-level lock is held across both
# the test and its cleanup, serialising tests that share a working directory even
# across separate ctest invocations or instances. The lock is released
# automatically when this process exits (including on crash), so a failed test
# cannot deadlock subsequent runs.

if(DEFINED LOCKFILE AND NOT LOCKFILE STREQUAL "")
    # file(LOCK) yields "0" on success or an error message otherwise.
    file(LOCK ${LOCKFILE} GUARD PROCESS RESULT_VARIABLE lock_result)
    if(NOT lock_result STREQUAL "0")
        message(FATAL_ERROR "Failed to acquire test lock ${LOCKFILE}: ${lock_result}")
    endif()
endif()

execute_process(COMMAND ${BASH} -e ${FILE_PATH}
    RESULT_VARIABLE test_result_${FILE_PATH}
    WORKING_DIRECTORY ${WORKING_DIRECTORY}
)
if(DEFINED CLEANUP_CMD AND NOT CLEANUP_CMD STREQUAL "")
    execute_process(COMMAND ${BASH} -c ${CLEANUP_CMD}
        WORKING_DIRECTORY ${WORKING_DIRECTORY}
    )
endif()

if(test_result_${FILE_PATH} EQUAL 0)
    message(STATUS "Test ${FILE_PATH} passed")
else()
    message(FATAL_ERROR "Test ${FILE_PATH} failed!")
endif()
