find_program(BASH bash)

set(DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)


### 5tt2gmwmi

add_test(
    NAME bintest_5tt2gmwmi
    COMMAND
    ${BASH} -c "$<TARGET_FILE:5tt2gmwmi>  ${DATA_DIR}/SIFT_phantom/5tt.mif - | $<TARGET_FILE:testing_diff_image> - ${DATA_DIR}/5tt2gmwmi/out.mif -abs 1e-3"
)


### 5tt2vis

add_test(
    NAME bintest_5tt2vis
    COMMAND
    ${BASH} -c "$<TARGET_FILE:5tt2vis>  ${DATA_DIR}/SIFT_phantom/5tt.mif - | $<TARGET_FILE:testing_diff_image> - ${DATA_DIR}/5tt2vis/out.mif -abs 1e-3"
)


### 5ttedit

add_test(
    NAME bintest_5ttedit
    COMMAND
    ${BASH} -c "$<TARGET_FILE:5ttedit> ${DATA_DIR}/SIFT_phantom/5tt.mif - -cgm ${DATA_DIR}/5ttedit/in0.mif -sgm ${DATA_DIR}/5ttedit/in1.mif -wm ${DATA_DIR}/5ttedit/in2.mif -csf ${DATA_DIR}/5ttedit/in3.mif -path ${DATA_DIR}/5ttedit/in4.mif -none ${DATA_DIR}/5ttedit/none.mif | $<TARGET_FILE:testing_diff_image> - ${DATA_DIR}/5ttedit/out.mif"
)


### amp2sh

add_test(
    NAME bintest_amp2sh
    COMMAND
    ${BASH} -c "$<TARGET_FILE:amp2sh> ${DATA_DIR}/dwi.mif - | $<TARGET_FILE:testing_diff_image> - ${DATA_DIR}/amp2sh/out.mif -voxel 1e-5;"
)


### connectomeedit

add_test(
    NAME bintest_connectomeedit_1
    COMMAND
    ${BASH} -c "printf $\"0.0 1.0 20.0\n0.0 10.0 0.0\n20.0 3.0 5.0\n\" > tmp.txt; $<TARGET_FILE:connectomeedit> -force tmp.txt to_symmetric tmp1.txt; printf $\"0.0 1.0 20.0\n1.0 10.0 3.0\n20.0 3.0 5.0\n\" > tmp2.txt; $<TARGET_FILE:testing_diff_matrix> -abs 1e-4 tmp1.txt tmp2.txt;"
)

add_test(
    NAME bintest_connectomeedit_2
    COMMAND
    ${BASH} -c "printf $\"0.0 1.0 20.0\n0.0 10.0 0.0\n20.0 3.0 5.0\n\" > tmp.txt; $<TARGET_FILE:connectomeedit> -force tmp.txt upper_triangular tmp1.txt; printf $\"0.0 1.0 20.0\n0.0 10.0 3.0\n0.0 0.0 5.0\n\" > tmp2.txt; $<TARGET_FILE:testing_diff_matrix> -abs 1e-4 tmp1.txt tmp2.txt;"
)

add_test(
    NAME bintest_connectomeedit_3
    COMMAND
    ${BASH} -c "printf $\"0.0 1.0 20.0\n0.0 10.0 0.0\n20.0 3.0 5.0\n\" > tmp.txt; $<TARGET_FILE:connectomeedit> -force tmp.txt lower_triangular tmp1.txt; printf $\"0.0 0.0 0.0\n1.0 10.0 0.0\n20.0 3.0 5.0\n\" > tmp.txt; $<TARGET_FILE:testing_diff_matrix> -abs 1e-4 tmp1.txt tmp2.txt;"
)

add_test(
    NAME bintest_connectomeedit_4
    COMMAND
    ${BASH} -c "printf $\"0.0 1.0 20.0\n0.0 10.0 0.0\n20.0 3.0 5.0\n\" > tmp.txt; $<TARGET_FILE:connectomeedit> -force tmp.txt transpose tmp1.txt; printf $\"0.0 0.0 20.0\n1.0 10.0 3.0\n20.0 0.0 5.0\n\" > tmp2.txt; $<TARGET_FILE:testing_diff_matrix> -abs 1e-4 tmp1.txt tmp2.txt;"
)

add_test(
    NAME bintest_connectomeedit_5
    COMMAND
    ${BASH} -c "printf $\"0.0 1.0 20.0\n0.0 10.0 0.0\n20.0 3.0 5.0\n\" > tmp.txt; $<TARGET_FILE:connectomeedit> -force tmp.txt zero_diagonal tmp1.txt; printf $\"0.0 1.0 20.0\n0.0 0.0 0.0\n20.0 3.0 0.0\n\" > tmp2.txt; $<TARGET_FILE:testing_diff_matrix> -abs 1e-4 tmp1.txt tmp2.txt;"
)
