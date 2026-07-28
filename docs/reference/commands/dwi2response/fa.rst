.. _dwi2response_fa:

dwi2response fa
===============

Synopsis
--------

Use the old FA-threshold heuristic for single-fibre voxel selection and response function estimation

Usage
-----

::

    dwi2response fa input output [ options ]

-  *input*: The input DWI
-  *output*: The output response function text file

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


Options specific to the "fa" algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-erode iterations** Number of brain mask erosion steps to apply prior to threshold (not used if mask is provided manually) (minimum: 0; default: 3)

-  **-number voxels** The number of highest-FA voxels to use (minimum: 1; default: 300)

-  **-threshold value** Apply a hard FA threshold, rather than selecting the top voxels (range: 0.0 to 1.0)

*(the options -number, -threshold are mutually exclusive; at most one may be specified)*


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

Tournier, J.-D.; Calamante, F.; Gadian, D. G. & Connelly, A. Direct estimation of the fiber orientation density function from diffusion-weighted MRI data using spherical deconvolution. NeuroImage, 2004, 23, 1176-1185

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

