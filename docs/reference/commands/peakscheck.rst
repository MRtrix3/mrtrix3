.. _peakscheck:

peakscheck
==========

Synopsis
--------

Check the orientations of an image containing discrete fibre orientations

Usage
-----

::

    peakscheck input [ options ]

-  *input*: The input fibre orientations image to be checked

Description
-----------

MRtrix3 expects "peaks" images to be stored using the real / scanner space axes as reference. There are three possible sources of error in this interpretation: 1. There may be erroneous axis flips and/or permutations, but within the real / scanner space reference. 2. The image data may provide fibre orientations with reference to the image axes rather than real / scanner space. Here there are two additional possibilities: 2a. There may be requisite axis permutations / flips to be applied to the image data *before* transforming them to real / scanner space. 2b. There may be requisite axis permutations / flips to be applied to the image data *after* transforming them to real / scanner space.

Options
-------

- **-mask image** Provide a mask image within which to seed & constrain tracking

- **-number value** Set the number of tracks to generate for each test

- **-threshold value** Modulate thresold on the ratio of empirical to maximal mean length to issue an error

- **-format choice** The format in which peak orientations are specified; one of: spherical,unitspherical,3vector,unit3vector

- **-reference choice** The a priori expected references axes against which the input orientations are defined; one of: xyz,ijk,fsl

- **-noshuffle** Do not evaluate possibility of requiring shuffles of axes or angles; only consider prospective transforms from alternative reference frames to real / scanner space

- **-notransform** Do not evaluate possibility of requiring transform of peak orientations from image to real / scanner space; only consider prospective shuffles of axes or angles

- **-all** Print table containing all results to standard output

- **-out_table file** Write text file with table containing all results

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify an existing directory in which to generate the scratch directory.

- **-continue ScratchDir LastFile** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

Standard options
^^^^^^^^^^^^^^^^

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages.

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

- **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

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

