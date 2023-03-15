.. _dwibiasnormmask:

dwibiasnormmask
===============

Synopsis
--------

Iteratively perform masking, RF estimation, CSD, bias field removal, and mask revision on a DWI series

Usage
-----

::

    dwibiasnormmask input output [ options ]

-  *input*: The input DWI series to be corrected
-  *output*: The output corrected image series

Description
-----------

DWI brain masking, response function estimation and bias field correction are inter-related steps and errors in each may influence other steps. This script first derives a brain mask (either with a provided algorithm  or by thresholding a balanced tissue sum image), and then performs response function estimation, multi-tissue CSD (with a lower lmax than the dwi2fod default, for speed), and mtnormalise to remove bias field, before the DWI brain mask is recalculated. These steps are performed iteratively until either a maximum number of iterations or until the brain masks converge. If SynthStrip is installed, it will be used to derive brain masks.

Options
-------

- **-output_mask** Output the final brain mask to the specified file

- **-max_iters** The maximum number of iterations. The default is 2iterations. More iterations may lead to a better mask, but for some problematic data this may lead to the masks diverging (in which case a warning is issued).

- **-lmax** The maximum spherical harmonic order for the output FODs. the value is passed to the dwi2fod command and should be provided in the format which it expects (the default value is "4,0,0" for multi-shell and "4,0" for single-shell data)

Options for importing the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-grad** Provide the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Provide the diffusion gradient table in FSL bvecs/bvals format

Options to specify an initial mask or tissue sum image threshold in the iterative algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-threshold** Threshold on the total tissue density image used to derive the brain mask. the default is 0.5

- **-mask_init image** Provide an initial mask to the algorithm and skip the initial masking

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify the path in which to generate the scratch directory.

- **-continue <ScratchDir> <LastFile>** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

Standard options
^^^^^^^^^^^^^^^^

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages.

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

- **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and Arshiya Sangchooli (asangchooli@student.unimelb.edu.au)

**Copyright:** Copyright (c) 2008-2023 the MRtrix3 contributors.

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

