.. _dwigradcheck:

dwigradcheck
============

Synopsis
--------

Check the orientation of the diffusion gradient table

Usage
--------

::

    dwigradcheck input [ options ]

-  *input*: The input DWI series to be checked

Options
-------

Options for the dwigradcheck script
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-mask** Provide a brain mask image

- **-number** Set the number of tracks to generate for each test

Options for importing the gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-grad** Provide a gradient table in MRtrix format

- **-fslgrad bvecs bvals** Provide a gradient table in FSL bvecs/bvals format

Options for exporting the estimated best gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-export_grad_mrtrix grad** Export the final gradient table in MRtrix format

- **-export_grad_fsl bvecs bvals** Export the final gradient table in FSL bvecs/bvals format

Options specific to Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete temporary files during script execution, and do not delete temporary directory at script completion

- **-tempdir /path/to/tmp/** manually specify the path in which to generate the temporary directory

- **-continue <TempDir> <LastFile>** continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file

Standard options
^^^^^^^^^^^^^^^^

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages.

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

* Jeurissen, B.; Leemans, A.; Sijbers, J. Automated correction of improperly rotated diffusion gradient orientations in diffusion weighted MRI. Medical Image Analysis, 2014, 18(7), 953-962

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/

