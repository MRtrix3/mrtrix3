file(
    GLOB CPP_COMMAND_FILES
    ${CMAKE_SOURCE_DIR}/cmd/*.cpp
)

file(
    GLOB PYTHON_ROOT_ENTRIES
    ${CMAKE_SOURCE_DIR}/python/mrtrix3/commands/*
)

set(MRTRIX_CPP_COMMAND_LIST "")
foreach(CPP_COMMAND_FILE ${CPP_COMMAND_FILES})
    get_filename_component(CPP_COMMAND_NAME ${CPP_COMMAND_FILE} NAME_WE)
    if(MRTRIX_CPP_COMMAND_LIST STREQUAL "")
        set(MRTRIX_CPP_COMMAND_LIST "\"${CPP_COMMAND_NAME}\"")
    else()
        set(MRTRIX_CPP_COMMAND_LIST "${MRTRIX_CPP_COMMAND_LIST},\n  \"${CPP_COMMAND_NAME}\"")
    endif()
endforeach()

set(MRTRIX_PYTHON_COMMAND_LIST "")
foreach(PYTHON_ROOT_ENTRY ${PYTHON_ROOT_ENTRIES})
    get_filename_component(PYTHON_COMMAND_NAME ${PYTHON_ROOT_ENTRY} NAME_WE)
    if(NOT ${PYTHON_COMMAND_NAME} STREQUAL "CMakeLists" AND NOT ${PYTHON_COMMAND_NAME} STREQUAL "__init__")
        if(MRTRIX_PYTHON_COMMAND_LIST STREQUAL "")
            set(MRTRIX_PYTHON_COMMAND_LIST "\"${PYTHON_COMMAND_NAME}\"")
        else()
            set(MRTRIX_PYTHON_COMMAND_LIST "${MRTRIX_PYTHON_COMMAND_LIST},\n  \"${PYTHON_COMMAND_NAME}\"")
        endif()
    endif()
endforeach()
message(VERBOSE "Completed GenPythonCommandsList() function")
message(VERBOSE "Formatted list of MRtrix3 C++ commands: ${MRTRIX_CPP_COMMAND_LIST}")
message(VERBOSE "Formatted list of MRtrix3 Python commands: ${MRTRIX_PYTHON_COMMAND_LIST}")

configure_file(
    ${SRC}
    ${DST}
    @ONLY
)
