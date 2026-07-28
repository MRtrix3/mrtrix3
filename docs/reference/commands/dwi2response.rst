.. _dwi2response:

dwi2response
============

Synopsis
--------

Estimate response function(s) for spherical deconvolution

Usage
-----

::

    dwi2response subcommand [ options ] ...

-  *subcommand*: Select the subcommand to be used; additional details and options become available once a subcommand is nominated. Options are: dhollander, fa, manual, msmt_5tt, tax, tournier

Description
-----------

dwi2response offers different algorithms for performing various types of response function estimation. The name of the sub-command must appear as the first argument on the command-line after "dwi2response". The subsequent arguments and options depend on the particular sub-command being invoked.

Each sub-command has its own help page, including necessary references; e.g. to see the help page of the "fa" sub-command, type "dwi2response fa".

More information on response function estimation for spherical deconvolution can be found at the following link: 
https://mrtrix.readthedocs.io/en/3.0.8/constrained_spherical_deconvolution/response_function_estimation.html

Note that if the -mask command-line option is not specified, the MRtrix3 command dwi2mask will automatically be called to derive an initial voxel exclusion mask. More information on mask derivation from DWI data can be found at: 
https://mrtrix.readthedocs.io/en/3.0.8/dwi_preprocessing/masking.html

In the absence of a user-specified mask (option -mask), the whole DWI series will be used for derivation of the brain mask, even where only a subset of the DWI volumes is used for response function estimation (whether because the -shells option has been specified, or because the nominated algorithm operates on only a single b-value shell). If it is instead desired that the same subset of shells used for response function estimation also be used for brain mask derivation, then the user has two alternatives: either generate that subset of shells themselves---e.g. using the dwiextract command---and provide the result as the input to dwi2response; or generate a brain mask from that subset of shells and provide that mask via the -mask option.

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

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and Thijs Dhollander (thijs.dhollander@gmail.com)

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

