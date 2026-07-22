.. _dwi2mask_fslbet:

dwi2mask fslbet
===============

Synopsis
--------

Use the FSL Brain Extraction Tool (bet) to generate a brain mask

Usage
-----

::

    dwi2mask fslbet input output [ options ]

-  *input*: The input DWI series
-  *output*: The output mask image

Options
-------

Options for importing the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion gradient table in MRtrix format

-  **-fslgrad bvecs bvals** Provide the diffusion gradient table in FSL bvecs/bvals format

*(these options are mutually exclusive; at most one may be specified)*


Options specific to the "fslbet" algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-bet_f value** Fractional intensity threshold (0->1); smaller values give larger brain outline estimates (range: 0.0 to 1.0)

-  **-bet_g value** Vertical gradient in fractional intensity threshold (-1->1); positive values give larger brain outline at bottom, smaller at top (range: -1.0 to 1.0)

-  **-bet_c i,j,k** Centre-of-gravity (voxels not mm) of initial mesh surface

-  **-bet_r value** Head radius (mm not voxels); initial surface sphere is set to half of this (minimum: 0.0)

-  **-rescale** Rescale voxel size provided to BET to 1mm isotropic; can improve results for rodent data

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

Smith, S. M. Fast robust automated brain extraction. Human Brain Mapping, 2002, 17, 143-155

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Warda Syeda (wtsyeda@unimelb.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)

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

