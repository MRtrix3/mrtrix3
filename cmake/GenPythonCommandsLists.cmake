# Run in `cmake -P` script mode, so set the policy version explicitly to enable the IN_LIST operator.
cmake_minimum_required(VERSION 3.22)

# MRTRIX_CMD_SUBSET is received as a comma-separated string (see commands/CMakeLists.txt); convert
# it back to a CMake list.
if(MRTRIX_CMD_SUBSET)
    string(REPLACE "," ";" MRTRIX_CMD_SUBSET "${MRTRIX_CMD_SUBSET}")
endif()

file(
    GLOB CPP_COMMAND_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp/cmd/*.cpp
)

file(
    GLOB PYTHON_ROOT_ENTRIES
    ${CMAKE_CURRENT_SOURCE_DIR}/python/mrtrix3/commands/*
)

# When MRTRIX_CMD_SUBSET is provided (external-project builds), only the named commands are
#   recorded in ALL_COMMANDS; otherwise every command is recorded (MRtrix3's own behaviour).
set(MRTRIX_CPP_COMMAND_LIST "")
foreach(CPP_COMMAND_FILE ${CPP_COMMAND_FILES})
    get_filename_component(CPP_COMMAND_NAME ${CPP_COMMAND_FILE} NAME_WE)
    if(MRTRIX_CMD_SUBSET AND NOT ${CPP_COMMAND_NAME} IN_LIST MRTRIX_CMD_SUBSET)
        continue()
    endif()
    list(APPEND MRTRIX_CPP_COMMAND_LIST "\"${CPP_COMMAND_NAME}\"")
endforeach()

set(MRTRIX_PYTHON_COMMAND_LIST "")
foreach(PYTHON_ROOT_ENTRY ${PYTHON_ROOT_ENTRIES})
    get_filename_component(PYTHON_COMMAND_NAME ${PYTHON_ROOT_ENTRY} NAME_WE)
    if(NOT ${PYTHON_COMMAND_NAME} STREQUAL "CMakeLists" AND NOT ${PYTHON_COMMAND_NAME} STREQUAL "__init__")
        if(MRTRIX_CMD_SUBSET AND NOT ${PYTHON_COMMAND_NAME} IN_LIST MRTRIX_CMD_SUBSET)
            continue()
        endif()
        list(APPEND MRTRIX_PYTHON_COMMAND_LIST "\"${PYTHON_COMMAND_NAME}\"")
    endif()
endforeach()

string(REPLACE ";" "," MRTRIX_CPP_COMMAND_LIST "${MRTRIX_CPP_COMMAND_LIST}")
string(REPLACE ";" "," MRTRIX_PYTHON_COMMAND_LIST "${MRTRIX_PYTHON_COMMAND_LIST}")

message(VERBOSE "Completed GenPythonCommandsList() function")
message(VERBOSE "Formatted list of MRtrix3 C++ commands: ${MRTRIX_CPP_COMMAND_LIST}")
message(VERBOSE "Formatted list of MRtrix3 Python commands: ${MRTRIX_PYTHON_COMMAND_LIST}")

configure_file(
    ${SRC}
    ${DST}
    @ONLY
)
