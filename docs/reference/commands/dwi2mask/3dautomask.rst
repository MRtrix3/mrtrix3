.. _dwi2mask_3dautomask:

dwi2mask 3dautomask
===================

Synopsis
--------

Use AFNI 3dAutomask to derive a brain mask from the DWI mean b=0 image

Usage
-----

::

    dwi2mask 3dautomask input output [ options ]

-  *input*: The input DWI series
-  *output*: The output mask image

Options
-------

Options for importing the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-grad file** Provide the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Provide the diffusion gradient table in FSL bvecs/bvals format

Options specific to the "3dautomask" algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-clfrac value** Set the "clip level fraction"; must be a number between 0.1 and 0.9. A small value means to make the initial threshold for clipping smaller, which will tend to make the mask larger. (range: 0.10000000000000001 to 0.90000000000000002)

- **-nograd** The program uses a "gradual" clip level by default. Add this option to use a fixed clip level.

- **-peels iterations** Peel (erode) the mask n times, then unpeel (dilate). (minimum: 0)

- **-nbhrs count** Define the number of neighbors needed for a voxel NOT to be eroded. It should be between 6 and 26. (range: 6 to 26)

- **-eclip** After creating the mask, remove exterior voxels below the clip threshold.

- **-SI value** After creating the mask, find the most superior voxel, then zero out everything more than SI millimeters inferior to that. 130 seems to be decent (i.e., for Homo Sapiens brains). (minimum: 0)

- **-dilate iterations** Dilate the mask outwards n times (minimum: 0)

- **-erode iterations** Erode the mask outwards n times (minimum: 0)

- **-NN1** Erode and dilate based on mask faces

- **-NN2** Erode and dilate based on mask edges

- **-NN3** Erode and dilate based on mask corners

Standard options
^^^^^^^^^^^^^^^^

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading). (minimum: 0)

- **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

- **-help** display this information page and exit.

- **-version** display version information and exit.

Verbosity options
"""""""""""""""""

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages & debug input data.

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify an existing directory in which to generate the scratch directory.

- **-continue ScratchDir LastFile** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

References
^^^^^^^^^^

* RW Cox. AFNI: Software for analysis and visualization of functional magnetic resonance neuroimages. Computers and Biomedical Research, 29:162-173, 1996.

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Ricardo Rios (ricardo.rios@cimat.mx)

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

