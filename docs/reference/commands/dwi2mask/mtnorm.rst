.. _dwi2mask_mtnorm:

dwi2mask mtnorm
===============

Synopsis
--------

Derives a DWI brain mask by calculating and then thresholding a sum-of-tissue-densities image

Usage
-----

::

    dwi2mask mtnorm input output [ options ]

-  *input*: The input DWI series
-  *output*: The output mask image

Description
-----------

This script attempts to identify brain voxels based on the total density of macroscopic tissues as estimated through multi-tissue deconvolution. Following response function estimation and multi-tissue deconvolution, the sum of tissue densities is thresholded at a fixed value (default is 0.5), and subsequent mask image cleaning operations are performed.

The operation of this script is a subset of that performed by the script "dwibiasnormmask". Many users may find that comprehensive solution preferable; this dwi2mask algorithm is nevertheless provided to demonstrate specifically the mask estimation portion of that command.

The ODFs estimated within this optimisation procedure are by default of lower maximal spherical harmonic degree than what would be advised for analysis. This is done for computational efficiency. This behaviour can be modified through the -lmax command-line option.

Options
-------

Options for importing the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion gradient table in MRtrix format

-  **-fslgrad bvecs bvals** Provide the diffusion gradient table in FSL bvecs/bvals format

*(these options are mutually exclusive; at most one may be specified)*


Options specific to the "mtnorm" algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-init_mask image** Provide an initial brain mask, which will constrain the response function estimation (if omitted, the default dwi2mask algorithm will be used)

-  **-lmax values** The maximum spherical harmonic degree for the estimated FODs (see Description); defaults are "4,0,0" for multi-shell and "4,0" for single-shell data (values must be non-negative and even)

-  **-threshold value** the threshold on the total tissue density sum image used to derive the brain mask; default is 0.5 (range: 0.0 to 1.0)

-  **-tissuesum image** Export the tissue sum image that was used to generate the mask

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

Jeurissen, B; Tournier, J-D; Dhollander, T; Connelly, A & Sijbers, J. Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. NeuroImage, 2014, 103, 411-426

Dhollander, T.; Raffelt, D. & Connelly, A. Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and Arshiya Sangchooli (asangchooli@student.unimelb.edu.au)

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

