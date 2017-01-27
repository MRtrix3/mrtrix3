.. _amp2response:

amp2response
===========

Synopsis
--------

::

    amp2response [ options ]  amps mask directions response

-  *amps*: the amplitudes image
-  *mask*: the mask containing the voxels from which to estimate the response function
-  *directions*: a 4D image containing the estimated fibre directions
-  *response*: the output zonal spherical harmonic coefficients

Description
-----------

Estimate response function coefficients based on the DWI signal in single-fibre voxels. This command uses the image data from all selected single-fibre voxels concurrently, rather than simply averaging their individual spherical harmonic coefficients. It also ensures that the response function is non-negative, and monotonic (i.e. its amplitude must increase from the fibre direction out to the orthogonal plane). If multi-shell data are provided, and one or more b-value shells are not explicitly requested, the command will generate a response function for every b-value shell (including b=0 if present).

Options
-------

-  **-isotropic** estimate an isotropic response function (lmax=0 for all shells)

-  **-noconstraint** disable the non-negativity and monotonicity constraints

-  **-directions path** provide an external text file containing the directions along which the amplitudes are sampled

DW Shell selection options
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-shell list** specify one or more diffusion-weighted gradient shells to use during processing, as a comma-separated list of the desired approximate b-values. Note that some commands are incompatible with multiple shells, and will throw an error if more than one b-value is provided.

-  **-lmax values** specify the maximum harmonic degree of the response function to estimate (can be a comma-separated list for multi-shell data)

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading)

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

