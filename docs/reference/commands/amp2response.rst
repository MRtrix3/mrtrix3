.. _amp2response:

amp2response
===================

Synopsis
--------

Estimate response function coefficients based on the DWI signal in single-fibre voxels

Usage
--------

::

    amp2response [ options ]  amps mask directions response

-  *amps*: the amplitudes image
-  *mask*: the mask containing the voxels from which to estimate the response function
-  *directions*: a 4D image containing the estimated fibre directions
-  *response*: the output zonal spherical harmonic coefficients

Description
-----------

This command uses the image data from all selected single-fibre voxels concurrently, rather than simply averaging their individual spherical harmonic coefficients. It also ensures that the response function is non-negative, and monotonic (i.e. its amplitude must increase from the fibre direction out to the orthogonal plane).

If multi-shell data are provided, and one or more b-value shells are not explicitly requested, the command will generate a response function for every b-value shell (including b=0 if present).

For details on the method provided by this command see: https://www.researchgate.net/publication/307862932_Constrained_linear_least_squares_estimation_of_anisotropic_response_function_for_spherical_deconvolution

Options
-------

-  **-isotropic** estimate an isotropic response function (lmax=0 for all shells)

-  **-noconstraint** disable the non-negativity and monotonicity constraints

-  **-directions path** provide an external text file containing the directions along which the amplitudes are sampled

DW shell selection options
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-shells b-values** specify one or more b-values to use during processing, as a comma-separated list of the desired approximate b-values (b-values are clustered to allow for small deviations). Note that some commands are incompatible with multiple b-values, and will report an error if more than one b-value is provided. WARNING: note that, even though the b=0 volumes are never referred to as shells in the literature, they still have to be explicitly included in the list of b-values as provided to the -shell option! Several algorithms which include the b=0 volumes in their computations may otherwise return an undesired result.

-  **-lmax values** specify the maximum harmonic degree of the response function to estimate (can be a comma-separated list for multi-shell data)

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

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


