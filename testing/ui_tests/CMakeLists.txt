include(GoogleTest)
# STL Debug flags like _GLIBCXX_DEBUG change the ABI for a target,
# this can cause issues when linking against gtest. To avoid this,
# we build gtest with the same flags as the mrtrix::common library.
target_compile_definitions(gtest PRIVATE
    $<TARGET_PROPERTY:mrtrix::common,INTERFACE_COMPILE_DEFINITIONS>
)

set(UI_TESTS_BASH_SRCS
    cpp_cli
    python_cli
)

get_filename_component(SOURCE_PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
set(DATA_DIR ${SOURCE_PARENT_DIR}/data)

find_program(BASH bash)

set(PYTHON_ENV_PATH "${PROJECT_BINARY_DIR}/lib")
# On MSYS2 we need to convert Windows paths to Unix paths
if(MINGW AND WIN32)
      EXECUTE_PROCESS(
          COMMAND cygpath -u ${PYTHON_ENV_PATH}
          OUTPUT_VARIABLE PYTHON_ENV_PATH
          OUTPUT_STRIP_TRAILING_WHITESPACE
      )
endif()

find_package(Python3)

include(DirsToUnix)
set(EXEC_DIRS ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND EXEC_DIRS ${PROJECT_BINARY_DIR}/bin)
list(APPEND EXEC_DIRS ${PROJECT_BINARY_DIR}/testing/tools)
list(APPEND EXEC_DIRS "$ENV{PATH}")
dirs_to_unix(EXEC_DIRS "${EXEC_DIRS}")


include(BashTests)
function (add_bash_ui_test FILE_SRC)
    get_filename_component(NAME ${FILE_SRC} NAME_WE)
    add_bash_test(
        FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${FILE_SRC}"
        PREFIX "uitest"
        WORKING_DIRECTORY ${DATA_DIR}
        EXEC_DIRECTORIES "${EXEC_DIRS}"
        ENVIRONMENT "PYTHONPATH=${PYTHON_ENV_PATH}"
        LABELS "cli;${NAME}"
    )
endfunction()

foreach(UI_TEST ${UI_TESTS_BASH_SRCS})
    add_bash_ui_test(${UI_TEST})
endforeach()
