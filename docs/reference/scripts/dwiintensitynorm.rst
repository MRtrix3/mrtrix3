.. _dwiintensitynorm:

dwiintensitynorm
================

Synopsis
--------

Performs a global DWI intensity normalisation on a group of subjects using the median b=0 white matter value as the reference

Usage
-----

::

    dwiintensitynorm input_dir mask_dir output_dir fa_template wm_mask [ options ]

-  *input_dir*: The input directory containing all DWI images
-  *mask_dir*: Input directory containing brain masks, corresponding to one per input image (with the same file name prefix)
-  *output_dir*: The output directory containing all of the intensity normalised DWI images
-  *fa_template*: The output population specific FA template, which is threshold to estimate a white matter mask
-  *wm_mask*: The output white matter mask (in template space), used to estimate the median b=0 white matter value for normalisation

Description
-----------

The white matter mask is estimated from a population average FA template then warped back to each subject to perform the intensity normalisation. Note that bias field correction should be performed prior to this step.

Options
-------

- **-fa_threshold** The threshold applied to the Fractional Anisotropy group template used to derive an approximate white matter mask (default: 0.4)

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

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

- **-help** display this information page and exit.

- **-version** display version information and exit.

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/

