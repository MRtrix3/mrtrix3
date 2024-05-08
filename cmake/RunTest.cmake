# This script can be invoked by calling ${CMAKE_COMMAND} -P <script_name>.cmake
# Runs a bash test described by the FILE_PATH variable
# The test is run in the WORKING_DIRECTORY directory
# The CLEANUP_CMD command is run after the test is finished

find_program(BASH bash)
execute_process(COMMAND ${BASH} -e ${FILE_PATH}
    RESULT_VARIABLE test_result_${FILE_PATH}
    WORKING_DIRECTORY ${WORKING_DIRECTORY}
)
execute_process(COMMAND ${CLEANUP_CMD}
    WORKING_DIRECTORY ${WORKING_DIRECTORY}
)
if(test_result_${FILE_PATH} EQUAL 0)
    message(STATUS "Test ${FILE_PATH} passed")
else()
    message(FATAL_ERROR "Test ${FILE_PATH} failed!")
endif()
