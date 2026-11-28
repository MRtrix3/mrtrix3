file(
    GLOB CPP_COMMAND_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp/cmd/*.cpp
)

file(
    GLOB PYTHON_ROOT_ENTRIES
    ${CMAKE_CURRENT_SOURCE_DIR}/python/mrtrix3/commands/*
)

set(MRTRIX_CPP_COMMAND_LIST "")
foreach(CPP_COMMAND_FILE ${CPP_COMMAND_FILES})
    get_filename_component(CPP_COMMAND_NAME ${CPP_COMMAND_FILE} NAME_WE)
    list(APPEND MRTRIX_CPP_COMMAND_LIST "\"${CPP_COMMAND_NAME}\"")
endforeach()

# Select commands by a positive criterion -- a single "<name>.py" file, or a package directory
#   containing an "__init__.py" -- so that build-system artefacts deposited alongside the sources by
#   an in-source build (e.g. the generated "CMakeFiles" directory, "cmake_install.cmake") are never
#   mistaken for commands.
set(MRTRIX_PYTHON_COMMAND_LIST "")
foreach(PYTHON_ROOT_ENTRY ${PYTHON_ROOT_ENTRIES})
    get_filename_component(PYTHON_ENTRY_NAME ${PYTHON_ROOT_ENTRY} NAME)
    if(IS_DIRECTORY ${PYTHON_ROOT_ENTRY})
        if(EXISTS "${PYTHON_ROOT_ENTRY}/__init__.py")
            list(APPEND MRTRIX_PYTHON_COMMAND_LIST "\"${PYTHON_ENTRY_NAME}\"")
        endif()
    elseif("${PYTHON_ENTRY_NAME}" MATCHES "\\.py$" AND NOT "${PYTHON_ENTRY_NAME}" STREQUAL "__init__.py")
        get_filename_component(PYTHON_COMMAND_NAME ${PYTHON_ROOT_ENTRY} NAME_WE)
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
