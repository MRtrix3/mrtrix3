dwi2response
===========

Synopsis
--------

::

    dwi2response [ options ]  dwi_in response_out

-  *dwi_in*: the input diffusion-weighted images
-  *response_out*: the output rotational harmonic coefficients

Description
-----------

generate an appropriate response function from the image data for spherical deconvolution

Options
-------

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad encoding** specify the diffusion-weighted gradient scheme used in the acquisition. The program will normally attempt to use the encoding stored in the image header. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2.

-  **-fslgrad bvecs bvals** specify the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format.

-  **-bvalue_scaling mode** specifies whether the b-values should be scaled by the square of the corresponding DW gradient norm, as often required for multi-shell or DSI DW acquisition schemes. The default action can also be set in the MRtrix config file, under the BValueScaling entry. Valid choices are yes/no, true/false, 0/1 (default: true).

DW Shell selection options
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-shell list** specify one or more diffusion-weighted gradient shells to use during processing, as a comma-separated list of the desired approximate b-values. Note that some commands are incompatible with multiple shells, and will throw an error if more than one b-value are provided.

-  **-mask image** provide an initial mask image

-  **-lmax value** specify the maximum harmonic degree of the response function to estimate

-  **-sf image** output a mask highlighting the final selection of single-fibre voxels

-  **-test_all** by default, only those voxels selected as single-fibre in the previous iteration are evaluated. Set this option to re-test all voxels at every iteration (slower).

Options for terminating the optimisation algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-max_iters value** maximum number of iterations per pass (set to zero to disable)

-  **-max_change value** maximum percentile change in any response function coefficient; if no individual coefficient changes by more than this fraction, the algorithm is terminated.

Thresholds for single-fibre voxel selection
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-volume_ratio value** maximal volume ratio between the sum of all other positive lobes in the voxel, and the largest FOD lobe (default = 0.15)

-  **-dispersion_multiplier value** dispersion of FOD lobe must not exceed some threshold as determined by this multiplier and the FOD dispersion in other single-fibre voxels. The threshold is: (mean + (multiplier * (mean - min))); default = 1.0. Criterion is only applied in second pass of RF estimation.

-  **-integral_multiplier value** integral of FOD lobe must not be outside some range as determined by this multiplier and FOD lobe integral in other single-fibre voxels. The range is: (mean +- (multiplier * stdev)); default = 2.0. Criterion is only applied in second pass of RF estimation.

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Tax, C. M.; Jeurissen, B.; Vos, S. B.; Viergever, M. A. & Leemans, A. Recursive calibration of the fiber response function for spherical deconvolution of diffusion MRI data. NeuroImage, 2014, 86, 67-80

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

