#!/bin/bash
set -ex 
mrcalc data/mrcalc/in.mif 2 -mult -neg -exp 10 -add - | diff_data - data/mrcalc/out1.mif 1.0e-15
mrcalc data/mrcalc/in.mif 1.224 -div -cos data/mrcalc/in.mif -abs -sqrt -log -atanh -sub - | diff_data - data/mrcalc/out2.mif 1.0e-15
mrcalc data/mrcalc/in.mif 0.2 -gt data/mrcalc/in.mif data/mrcalc/in.mif -1.123 -mult 0.9324 -add -exp -neg -if - | diff_data - data/mrcalc/out3.mif 1.0e-15
mrcalc data/mrcalc/in.mif 0+1i -mult -exp data/mrcalc/in.mif -mult 1.34+5.12i -mult - | diff_data - data/mrcalc/out4.mif 1.0e-15

