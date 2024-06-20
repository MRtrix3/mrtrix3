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

set(PYTHON_TOOLS_SRCS
    testing_check_npy.py
    testing_gen_npy.py
    testing_python_cli.py
)

add_custom_target(testing_tools ALL)

function(add_cpp_tool TOOL_SRC)
    # Extract the filename without an extension (NAME_WE)
    get_filename_component(TOOL_NAME ${TOOL_SRC} NAME_WE)
    add_executable(${TOOL_NAME} ${TOOL_SRC})
    target_link_libraries(${TOOL_NAME} PRIVATE
        mrtrix::headless
        mrtrix::exec-version-lib
        mrtrix::tests-lib
    )
    set_target_properties(${TOOL_NAME} PROPERTIES LINK_DEPENDS_NO_SHARED true)
    add_dependencies(testing_tools ${TOOL_NAME})
endfunction()

function(add_python_tool TOOL_SRC IS_MRTRIX_CMD)
    get_filename_component(TOOL_NAME ${TOOL_SRC} NAME_WE)
    # If the tool behaves like a Python MRtrix3 command, then we need to generate its executable using
    # the MakePythonExecutable script
    if(IS_MRTRIX_CMD)
        set(MRTRIX_PYTHON_LIB_PATH "${PROJECT_BINARY_DIR}/lib")
        add_custom_command(
            TARGET testing_tools PRE_BUILD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMAND ${CMAKE_COMMAND} -DCMDNAME=${TOOL_NAME} -DOUTPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}" -DEXTRA_PATH_DIRS="${MRTRIX_PYTHON_LIB_PATH}" -P ${PROJECT_SOURCE_DIR}/cmake/MakePythonExecutable.cmake
        )
        # Install "module" file alongside the executable
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TOOL_SRC} ${CMAKE_CURRENT_BINARY_DIR}/${TOOL_SRC} COPYONLY)
    else()
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${TOOL_SRC} ${CMAKE_CURRENT_BINARY_DIR}/${TOOL_NAME} COPYONLY)
    endif()
endfunction()

foreach(CMD ${CPP_TOOLS_SRCS})
    add_cpp_tool(${CMD})
endforeach(CMD)

add_python_tool(testing_check_npy.py FALSE)
add_python_tool(testing_gen_npy.py FALSE)
add_python_tool(testing_python_cli.py TRUE)
