find_program(BASH bash)

set(DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)

add_test(
    NAME 5tt2gmwmi
    COMMAND
    ${BASH} -c '$<TARGET_FILE:5tt2gmwmi>  ${DATA_DIR}/SIFT_phantom/5tt.mif - | $<TARGET_FILE:testing_diff_image> - ${DATA_DIR}/5tt2gmwmi/out.mif -abs 1e-3'
)

add_test(
    NAME 5tt2vis
    COMMAND
    ${BASH} -c '$<TARGET_FILE:5tt2vis>  ${DATA_DIR}/SIFT_phantom/5tt.mif - | $<TARGET_FILE:testing_diff_image> - ${DATA_DIR}/5tt2vis/out.mif -abs 1e-3'
)

add_test(
    NAME 5ttedit
    COMMAND
    ${BASH} -c '$<TARGET_FILE:5ttedit> ${DATA_DIR}/SIFT_phantom/5tt.mif - -cgm ${DATA_DIR}/5ttedit/in0.mif -sgm ${DATA_DIR}/5ttedit/in1.mif -wm ${DATA_DIR}/5ttedit/in2.mif -csf ${DATA_DIR}/5ttedit/in3.mif -path ${DATA_DIR}/5ttedit/in4.mif -none ${DATA_DIR}/5ttedit/none.mif | $<TARGET_FILE:testing_diff_image> - ${DATA_DIR}/5ttedit/out.mif'
)

add_test(
    NAME amp2sh
    COMMAND
    ${BASH} -c '$<TARGET_FILE:amp2sh> ${DATA_DIR}/dwi.mif - | $<TARGET_FILE:testing_diff_image> - ${DATA_DIR}/amp2sh/out.mif -voxel 1e-5'
)
