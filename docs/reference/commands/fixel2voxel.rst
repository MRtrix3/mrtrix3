.. _fixel2voxel:

fixel2voxel
===========

Synopsis
--------

::

    fixel2voxel [ options ]  fixel_in operation image_out

-  *fixel_in*: the input sparse fixel image.
-  *operation*: the operation to apply, one of: mean, sum, product, rms, var, std, min, max, absmax, magmax, count, complexity, sf, dec_unit, dec_scaled, split_size, split_value, split_dir.
-  *image_out*: the output scalar image.

Description
-----------

convert a fixel-based sparse-data image into some form of scalar image. This could be: - Some statistic computed across all fixel values within a voxel: mean, sum, product, rms, var, std, min, max, absmax, magmax- The number of fixels in each voxel: count- Some measure of crossing-fibre organisation: complexity, sf ('single-fibre')- A 4D directionally-encoded colour image: dec_unit, dec_scaled- A 4D scalar image with one 3D volume per fixel: split_size, split_value- A 4D image with three 3D volumes per fixel direction: split_dir

Options
-------

-  **-weighted** weight the contribution of each fixel to the per-voxel result according to its volume (note that this option is not applicable for all operations, and should be avoided if the value stored in the fixel image is itself the estimated fibre volume)

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

References
^^^^^^^^^^

* Reference for 'complexity' operation:Riffert, T. W.; Schreiber, J.; Anwander, A. & Knosche, T. R. Beyond Fractional Anisotropy: Extraction of bundle-specific structural metrics from crossing fibre models. NeuroImage, 2014 (in press)

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

