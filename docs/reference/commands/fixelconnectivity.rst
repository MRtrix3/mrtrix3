.. _fixelconnectivity:

fixelconnectivity
===================

Synopsis
--------

Generate one or more fixel-fixel connectivity matrices

Usage
--------

::

    fixelconnectivity [ options ]  fixel_directory tracks matrix

-  *fixel_directory*: the directory containing the fixels between which connectivity will be quantified
-  *tracks*: the tracks used to determine fixel-fixel connectivity
-  *matrix*: the output fixel-fixel connectivity matrix

Description
-----------

If this command is used within a typical Fixel-Based Analysis (FBA) pipeline, one would be expected to utilise the -smoothing command-line option, as this will provide the matrix by which fixel data should be smoothed in template space, as opposed to statistical testing which would typically use the full output connectivity matrix

Options
-------

Options for generating an additional connectivity matrix
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-smoothing path** generate a second fixel-fixel connectivity matrix that incorporates both streamlines-based connectivity and spatial distance, which is typically used for smoothing fixel-based data

-  **-fwhm value** manually specify the full-width half-maximum of the fixel data smoothing matrix (default: 10mm)

Options that influence generation of the connectivity matrix / matrices
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-threshold value** a threshold to define the required fraction of shared connections to be included in the neighbourhood (default: 0.01)

-  **-angle value** the max angle threshold for assigning streamline tangents to fixels (Default: 45 degrees)

-  **-mask file** provide a fixel data file containing a mask of those fixels to be computed; fixels outside the mask will be empty in the output matrix

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


