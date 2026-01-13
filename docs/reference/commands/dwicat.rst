.. _dwicat:

dwicat
======

Synopsis
--------

Concatenating multiple DWI series accounting for differential intensity scaling

Usage
-----

::

    dwicat inputs output [ options ]

-  *inputs*: Multiple input diffusion MRI series
-  *output*: The output image series (all DWIs concatenated)

Description
-----------

This script concatenates two or more 4D DWI series, accounting for various detrimental confounds that may affect such an operation. Each of those confounds are described in separate paragraphs below.

There may be differences in intensity scaling between the input series. This intensity scaling is corrected by determining scaling factors that will make the overall image intensities in the b=0 volumes of each series approximately equivalent. This operation is only appropriate if the sequence parameters that influence the contrast of the b=0 image volumes are identical.

Concatenation of DWI series defined on different voxel grids. If the voxel grids of the input DWI series are not precisely identical, then it may not be possible to do a simple concatenation operation. In this scenario the script will determine the appropriate way to combine the input series, ideally only manipulating header transformations and avoiding image interpolation if possible.

Options
-------

- **-mask image** Provide a binary mask within which image intensities will be matched

- **-nointensity** Do not perform intensity matching based on b=0 volumes

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify an existing directory in which to generate the scratch directory.

- **-continue ScratchDir LastFile** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

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



**Author:** Lena Dorfschmidt (ld548@cam.ac.uk) and Jakub Vohryzek (jakub.vohryzek@queens.ox.ac.uk) and Robert E. Smith (robert.smith@florey.edu.au)

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

