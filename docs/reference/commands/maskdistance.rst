.. _maskdistance:

maskdistance
===================

Synopsis
--------

Map the minimal distance to a mask along streamlines trajectories

Usage
--------

::

    maskdistance [ options ]  roi tracks output

-  *roi*: the region of interest mask
-  *tracks*: the input track file
-  *output*: the output path (either an image, a fixel data file, or a Track Scalar File (.tsf)

Description
-----------

This command aims to determine the spatial distance from any location to a user-defined 3D binary mask, but where those distances are computed based on the length along streamline trajectories rather than simple Euclidean distances.

It is not necssary for the input ROI to be a single spatially contiguous cluster. For every streamline vertex, the distance to the nearest vertex that intersects the ROI is quantified; this could be in either direction along the trajectory of that particular streamline.

Any streamline that does not intersect the input ROI will not contribute in any way to the resulting distance maps.

The command can be used in one of three ways:

- By providing an input image via the -template option, the output image (defined on the same image grid) will contain, for every voxel, a value of minimum distance to the ROI based on the mean across those streamlines that intersect both that voxel and the ROI.

- By providing either a fixel directory or fixel data file via the -template option, the output will be a fixel data file that contains, for every fixel, a value of minimum distance to the ROI based on the mean across those streamlines that intersect both that fixel and the ROI.

- By not providing the -template option, the output will be a Track Scalar File (.tsf) that contains, for every streamline vertex, the minimal distance to the ROI along the trajectory of that streamline. Any streamline that does not intersect the ROI at any point will contain a value of -1.0 at every vertex.

Options
-------

-  **-template image** template on which an output image will be based; this can be either a >=3D image, or a fixel directory / file within such

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2023 the MRtrix3 contributors.

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


