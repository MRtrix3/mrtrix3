set(CPP_TOOLS_SRCS
    testing_cpp_cli.cpp
    testing_diff_dir.cpp
    testing_diff_fixel.cpp
    testing_diff_fixel_old.cpp
    testing_diff_header.cpp
    testing_diff_image.cpp
    testing_diff_matrix.cpp
    testing_diff_mesh.cpp
    testing_diff_peaks.cpp
    testing_diff_tck.cpp
    testing_diff_tsf.cpp
    testing_gen_data.cpp
    testing_npyread.cpp
    testing_npywrite.cpp
)

set(PYTHON_TOOLS_STANDALONE
    testing_check_npy.py
    testing_gen_npy.py
)

set(PYTHON_TOOLS_MRTRIXAPI
    testing_python_cli.py
)

add_custom_target(testing_tools ALL)

function(add_cpp_tool TOOL_SRC)
    # Extract the filename without an extension (NAME_WE)
    get_filename_component(TOOL_NAME ${TOOL_SRC} NAME_WE)
    add_executable(${TOOL_NAME} ${TOOL_SRC})
    target_link_libraries(${TOOL_NAME} PRIVATE
        mrtrix::core
        mrtrix::executable-version
        mrtrix::tests-lib
    )
    set_target_properties(${TOOL_NAME} PROPERTIES LINK_DEPENDS_NO_SHARED true)
    add_dependencies(testing_tools ${TOOL_NAME})
endfunction()

function(add_python_tool_standalone TOOL_SRC)
    get_filename_component(TOOL_NAME ${TOOL_SRC} NAME_WE)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TOOL_SRC} ${CMAKE_CURRENT_BINARY_DIR}/${TOOL_NAME} COPYONLY)
endfunction()

# If the tool behaves like a Python MRtrix3 command, then we need to generate its executable using
# the MakePythonExecutable script
function(add_python_tool_mrtrixapi TOOL_SRC)
    get_filename_component(TOOL_NAME ${TOOL_SRC} NAME_WE)
    set(MRTRIX_PYTHON_LIB_PATH "${PROJECT_BINARY_DIR}/lib")
    add_custom_command(
        TARGET testing_tools PRE_BUILD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -DCMDNAME=${TOOL_NAME} -DOUTPUT_DIR="${PROJECT_BINARY_DIR}/bin" -P ${PROJECT_SOURCE_DIR}/cmake/MakePythonExecutable.cmake
    )
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TOOL_SRC} ${PROJECT_BINARY_DIR}/lib/mrtrix3/commands/${TOOL_SRC} COPYONLY)
endfunction()

foreach(CMD ${CPP_TOOLS_SRCS})
    add_cpp_tool(${CMD})
endforeach(CMD)

foreach(CMD ${PYTHON_TOOLS_STANDALONE})
    add_python_tool_standalone(${CMD})
endforeach(CMD)

foreach(CMD ${PYTHON_TOOLS_MRTRIXAPI})
    add_python_tool_mrtrixapi(${CMD})
endforeach(CMD)
