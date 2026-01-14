include(GoogleTest)
# STL Debug flags like _GLIBCXX_DEBUG change the ABI for a target,
# this can cause issues when linking against gtest. To avoid this,
# we build gtest with the same flags as the mrtrix::common library.
target_compile_definitions(gtest PRIVATE
    $<TARGET_PROPERTY:mrtrix::common,INTERFACE_COMPILE_DEFINITIONS>
)

set(UNIT_TESTS_CPP_SRCS
    erfinv_tests.cpp
    icls_tests.cpp
    ordered_queue_tests.cpp
    parse_axes_tests.cpp
    parse_ints_tests.cpp
    platform_tests.cpp
    roi_tests.cpp
    sh_precomputer_tests.cpp
    shuffle_tests.cpp
    to_tests.cpp
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

find_package(Threads REQUIRED)

include(DirsToUnix)
set(EXEC_DIRS ${CMAKE_CURRENT_BINARY_DIR})
list(APPEND EXEC_DIRS ${PROJECT_BINARY_DIR}/bin)
list(APPEND EXEC_DIRS ${PROJECT_BINARY_DIR}/testing/tools)
list(APPEND EXEC_DIRS "$ENV{PATH}")
dirs_to_unix(EXEC_DIRS "${EXEC_DIRS}")


add_executable(mrtrix-unit-tests
    ${UNIT_TESTS_CPP_SRCS}
)
set_target_properties(mrtrix-unit-tests PROPERTIES
    LINK_DEPENDS_NO_SHARED true
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)
target_link_libraries(mrtrix-unit-tests PRIVATE
    mrtrix::core
    mrtrix::executable-version
    mrtrix::common
    GTest::gtest_main
    Threads::Threads
)
target_compile_definitions(mrtrix-unit-tests PRIVATE
    # To avoid conflicts with MRtrix's FAIL macro in exception.h
    GTEST_DONT_DEFINE_FAIL
)
gtest_discover_tests(mrtrix-unit-tests
    PROPERTIES LABELS "unittest"
)

include(BashTests)
function (add_bash_unit_test FILE_SRC)
    get_filename_component(NAME ${FILE_SRC} NAME_WE)
    add_bash_test(
        FILE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${FILE_SRC}"
        PREFIX "unittest"
        WORKING_DIRECTORY ${DATA_DIR}
        EXEC_DIRECTORIES "${EXEC_DIRS}"
        LABELS "unittest;${NAME}"
    )
endfunction()
