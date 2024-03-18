file(
    GLOB CPP_COMMAND_FILES
    ${CMAKE_SOURCE_DIR}/cmd/*.cpp
)

file(
    GLOB PYTHON_ROOT_ENTRIES
    ${CMAKE_SOURCE_DIR}/python/mrtrix3/*/
)

set(MRTRIX_COMMAND_LIST "")
foreach(CPP_COMMAND_FILE ${CPP_COMMAND_FILES})
    get_filename_component(CPP_COMMAND_NAME ${CPP_COMMAND_FILE} NAME_WE)
    if(MRTRIX_COMMAND_LIST STREQUAL "")
        set(MRTRIX_COMMAND_LIST "\"${CPP_COMMAND_NAME}\"")
    else()
        set(MRTRIX_COMMAND_LIST "${MRTRIX_COMMAND_LIST},\n                \"${CPP_COMMAND_NAME}\"")
    endif()
endforeach()
foreach(PYTHON_ROOT_ENTRY ${PYTHON_ROOT_ENTRIES})
    if(IS_DIRECTORY ${PYTHON_ROOT_ENTRY})
        get_filename_component(PYTHON_COMMAND_NAME ${PYTHON_ROOT_ENTRY} NAME)
        set(MRTRIX_COMMAND_LIST "${MRTRIX_COMMAND_LIST},\n                \"${PYTHON_COMMAND_NAME}\"")
    endif()
endforeach()
set(MRTRIX_COMMAND_PATH ${CMAKE_CURRENT_BUILD_DIR}/bin)
message(VERBOSE "Completed FindCommands() function")
message(VERBOSE "MRtrix3 executables location: ${MRTRIX_COMMAND_PATH}")
message(VERBOSE "List of MRtrix3 commands: ${MRTRIX_COMMAND_LIST}")

configure_file(
    ${SRC}
    ${DST}
    @ONLY
)
