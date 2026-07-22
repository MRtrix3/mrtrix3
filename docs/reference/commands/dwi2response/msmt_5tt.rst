.. _dwi2response_msmt_5tt:

dwi2response msmt_5tt
=====================

Synopsis
--------

Derive MSMT-CSD tissue response functions based on a co-registered five-tissue-type (5TT) image

Usage
-----

::

    dwi2response msmt_5tt input in_5tt out_wm out_gm out_csf [ options ]

-  *input*: The input DWI
-  *in_5tt*: Input co-registered 5TT image
-  *out_wm*: Output WM response text file
-  *out_gm*: Output GM response text file
-  *out_csf*: Output CSF response text file

Options
-------

General dwi2response options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-mask image** Provide an initial mask for response voxel selection

-  **-voxels image** Output an image showing the final voxel selection(s)

-  **-shells bvalues** The b-value(s) to use in response function estimation (comma-separated list in case of multiple b-values; b=0 must be included explicitly if desired)

-  **-lmax values** The maximum harmonic degree(s) for response function estimation (comma-separated list in case of multiple b-values) (values must be non-negative and even)

Options for importing the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion gradient table in MRtrix format

-  **-fslgrad bvecs bvals** Provide the diffusion gradient table in FSL bvecs/bvals format

*(these options are mutually exclusive; at most one may be specified)*


Options specific to the "msmt_5tt" algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-dirs image** Provide an input image that contains a pre-estimated fibre direction in each voxel (a tensor fit will be used otherwise)

-  **-fa value** Upper fractional anisotropy threshold for GM and CSF voxel selection (range: 0.0 to 1.0; default: 0.20000000000000001)

-  **-pvf fraction** Partial volume fraction threshold for tissue voxel selection (range: 0.0 to 1.0; default: 0.94999999999999996)

-  **-wm_algo algorithm** dwi2response algorithm to use for WM single-fibre voxel selection (choices: fa, tax, tournier; default: tournier)

-  **-sfwm_fa_threshold value** Sets -wm_algo to fa and allows to specify a hard FA threshold for single-fibre WM voxels, which is passed to the -threshold option of the fa algorithm (warning: overrides -wm_algo option) (range: 0.0 to 1.0)

Standard options
^^^^^^^^^^^^^^^^

-  **-force** force overwrite of output files.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading). (minimum: 0)

-  **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

Verbosity options
"""""""""""""""""

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages & debug input data.

*(these options are mutually exclusive; at most one may be specified)*


Additional standard options for Python scripts
""""""""""""""""""""""""""""""""""""""""""""""

-  **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

-  **-scratch /path/to/scratch/** manually specify an existing directory in which to generate the scratch directory.

-  **-continue ScratchDir LastFile** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

References
^^^^^^^^^^

Jeurissen, B.; Tournier, J.-D.; Dhollander, T.; Connelly, A. & Sijbers, J. Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. NeuroImage, 2014, 103, 411-426

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2026 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Covered Software is provided under this License on an "as is"
basis, without warranty of any kind, either expressed, implied, or
statutory, including, without limitation, warranties that the
Covered Software is free of defects, merchantable, fit for a
particular purpose or non-infringing.
See the Mozilla Public License v. 2.0 for more details.

For more details, see http://www.mrtrix.org/.

