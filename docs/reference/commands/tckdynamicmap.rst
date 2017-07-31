.. _tckdynamicmap:

tckdynamicmap
===================

Synopsis
--------

Perform the Track-Weighted Dynamic Functional Connectivity (TW-dFC) method.

Usage
--------

::

    tckdynamicmap [ options ]  tracks fmri output

-  *tracks*: the input track file.
-  *fmri*: the pre-processed fMRI time series
-  *output*: the output TW-dFC image

Description
-----------

This command generates a sliding-window Track-Weighted Image (TWI), where the contribution from each streamline to the image at each timepoint is the Pearson correlation between the fMRI time series at the streamline endpoints, within a sliding temporal window centred at that timepoint.

Options
-------

-  **-template image** an image file to be used as a template for the output (the output image will have the same transform and field of view).

-  **-vox size** provide either an isotropic voxel size (in mm), or comma-separated list of 3 voxel dimensions.

-  **-stat_vox type** define the statistic for choosing the final voxel intensities for a given contrast type given the individual values from the tracks passing through each voxelOptions are: sum, min, mean, max (default: mean)

-  **-window_shape shape** specify the shape of the sliding window weighting function. Options are: rectangle, triangle, cosine, hann, hamming, lanczos (default = rectangle)

-  **-window_width value** set the full width of the sliding window (in volumes, not time) (must be an odd number) (default = 15)

-  **-backtrack** if no valid timeseries is found at the streamline endpoint, backtrack along the streamline trajectory until a valid timeseries is found

-  **-upsample factor** upsample the tracks by some ratio using Hermite interpolation before mappipng (if omitted, an appropriate ratio will be determined automatically)

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

Calamante, F.; Smith, R.E.; Liang, X.; Zalesky, A.; Connelly, A Track-weighted dynamic functional connectivity (TW-dFC): a new method to study time-resolved functional connectivity. Brain Struct Funct, 2017, doi: 10.1007/s00429-017-1431-1

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


