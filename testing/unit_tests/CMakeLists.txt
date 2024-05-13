set(UNIT_TESTS_CPP_SRCS
    bitset.cpp
    erfinv.cpp
    icls.cpp
    ordered_include.cpp
    ordered_queue.cpp
    parse_ints.cpp
    sh_precomputer.cpp
    shuffle.cpp
    to.cpp
)

set(UNIT_TESTS_BASH_SRCS
    cpp_cli
    python_cli
)
set(UNIT_TESTS_NPY_BASH_SRCS
    npyread
    npywrite
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

find_package(Python3
    COMPONENTS NumPy
)

include(DirsToUnix)
set(EXEC_DIRS ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND EXEC_DIRS ${PROJECT_BINARY_DIR}/bin)
list(APPEND EXEC_DIRS ${PROJECT_BINARY_DIR}/testing/tools)
list(APPEND EXEC_DIRS "$ENV{PATH}")
dirs_to_unix(EXEC_DIRS "${EXEC_DIRS}")

function(add_cpp_unit_test FILE_SRC)
    get_filename_component(NAME ${FILE_SRC} NAME_WE)
    add_executable(${NAME} ${FILE_SRC})
    target_link_libraries(${NAME} PRIVATE
        mrtrix::headless
        mrtrix::exec-version-lib
        mrtrix::tests-lib
    )
    set_target_properties(${NAME} PROPERTIES
        LINK_DEPENDS_NO_SHARED true
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
    )
    add_test(
        NAME unittest_${NAME}
        COMMAND ${BASH} -c "$<TARGET_FILE:${NAME}>"
    )
    set_tests_properties(unittest_${NAME} PROPERTIES LABELS "unittest")
endfunction()

function (add_bash_unit_test FILE_SRC)
    get_filename_component(NAME ${FILE_SRC} NAME_WE)
    add_bash_test(
        FILE_PATH "${FILE_SRC}"
        PREFIX "unittest"
        WORKING_DIRECTORY ${DATA_DIR}
        EXEC_DIRECTORIES "${EXEC_DIRS}"
        ENVIRONMENT "PYTHONPATH=${PYTHON_ENV_PATH}"
        LABELS "unittest"
    )
endfunction()

foreach(UNIT_TEST ${UNIT_TESTS_CPP_SRCS})
    add_cpp_unit_test(${UNIT_TEST})
endforeach()

foreach(UNIT_TEST ${UNIT_TESTS_BASH_SRCS})
    add_bash_unit_test(${UNIT_TEST})
endforeach()

if(${Python3_NumPy_FOUND})
    foreach(UNIT_TEST ${UNIT_TESTS_NPY_BASH_SRCS})
        add_bash_unit_test(${UNIT_TEST})
    endforeach()
else()
    message(WARNING "Python numpy module not found; omitting .npy format support tests")
endif()
