set(CPP_TOOLS_SRCS
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
    testing_check_npy
    testing_gen_npy
)

function(add_testing_cmd CMD_SRC)
    # Extract the filename without an extension (NAME_WE)
    get_filename_component(CMD_NAME ${CMD} NAME_WE)
    add_executable(${CMD_NAME} ${CMD})
    target_link_libraries(${CMD_NAME} PRIVATE
        mrtrix::headless
        mrtrix::exec-version-lib
        mrtrix::tests-lib
    )
    set_target_properties(${CMD_NAME} PROPERTIES LINK_DEPENDS_NO_SHARED true)
endfunction()


foreach(CMD ${CPP_TOOLS_SRCS})
    add_testing_cmd(${CMD})
endforeach(CMD)

foreach(CMD ${PYTHON_TOOLS_SRCS})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${CMD} ${CMAKE_CURRENT_BINARY_DIR}/${CMD} COPYONLY)
endforeach(CMD)
