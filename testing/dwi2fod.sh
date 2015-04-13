dwi2fod dwi.mif response.txt - | testing_diff_data - dwi2fod/out.mif 1e-15
dwi2fod dwi.mif response.txt -lmax 12 - | testing_diff_data - dwi2fod/out_lmax12.mif 1e-15
