find_program(BASH bash)

set(DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)

include(BashTests)
function(add_bash_script_tests file_path)
    set(EXEC_DIRS ${CMAKE_CURRENT_BINARY_DIR})
    list(APPEND EXEC_DIRS ${PROJECT_BINARY_DIR}/bin)
    list(APPEND EXEC_DIRS ${PROJECT_BINARY_DIR}/testing/tools)

    add_bash_tests(
        FILE_PATH "${file_path}"
        PREFIX "script"
        WORKING_DIRECTORY ${DATA_DIR}
        EXEC_DIRECTORIES "${EXEC_DIRS}"
    )
endfunction()

add_bash_script_tests(${CMAKE_CURRENT_SOURCE_DIR}/tests/5ttgen)
add_bash_script_tests(${CMAKE_CURRENT_SOURCE_DIR}/tests/dwi2mask)
add_bash_script_tests(${CMAKE_CURRENT_SOURCE_DIR}/tests/dwi2response)
add_bash_script_tests(${CMAKE_CURRENT_SOURCE_DIR}/tests/dwibiascorrect)
add_bash_script_tests(${CMAKE_CURRENT_SOURCE_DIR}/tests/dwicat)
add_bash_script_tests(${CMAKE_CURRENT_SOURCE_DIR}/tests/dwifslpreproc)
add_bash_script_tests(${CMAKE_CURRENT_SOURCE_DIR}/tests/dwigradcheck)
add_bash_script_tests(${CMAKE_CURRENT_SOURCE_DIR}/tests/dwinormalise)
add_bash_script_tests(${CMAKE_CURRENT_SOURCE_DIR}/tests/dwishellmath)
add_bash_script_tests(${CMAKE_CURRENT_SOURCE_DIR}/tests/for_each) 
add_bash_script_tests(${CMAKE_CURRENT_SOURCE_DIR}/tests/labelsgmfix)
add_bash_script_tests(${CMAKE_CURRENT_SOURCE_DIR}/tests/population_template)
add_bash_script_tests(${CMAKE_CURRENT_SOURCE_DIR}/tests/responsemean)