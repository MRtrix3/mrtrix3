.. _dirstat:

dirstat
===================

Synopsis
--------

Report statistics on a direction set

Usage
--------

::

    dirstat [ options ]  dirs

-  *dirs*: the text file or image containing the directions.

Description
-----------

This command will accept as inputs:

- directions file in spherical coordinates (ASCII text, [ az el ] space-separated values, one per line);

- directions file in Cartesian coordinates (ASCII text, [ x y z ] space-separated values, one per line);

- DW gradient files (MRtrix format: ASCII text, [ x y z b ] space-separated values, one per line);

- image files, using the DW gradient scheme found in the header (or provided using the appropriate command line options below).

By default, this produces all relevant metrics for the direction set provided. If the direction set contains multiple shells, metrics are provided for each shell separately.

Metrics are produced assuming a unipolar or bipolar electrostatic repulsion model, producing the potential energy (total, mean, min & max), and the nearest-neighbour angles (mean, min & max). The condition number is also produced for the spherical harmonic fits up to the highest harmonic order supported by the number of volumes. Finally, the norm of the mean direction vector is provided as a measure of the overall symmetry of the direction set (important with respect to eddy-current resilience).

Specific metrics can also be queried independently via the "-output" option, using these shorthands:  |br|
U/B for unipolar/bipolar model,  |br|
E/N for energy and nearest-neighbour respectively,  |br|
t/-/+ for total/min/max respectively (mean implied otherwise);  |br|
SHn for condition number of SH fit at order n (with n an even integer);  |br|
ASYM for asymmetry index (norm of mean direction vector);  |br|
N for the number of directions.

Example usages
--------------

-   *Default usage*::

        $ dirstat directions.txt

    This provides a pretty-printed list of all metrics available.

-   *Write a single metric of interest to standard output*::

        $ dirstat grad.b -shell 3000 -output SH8

    requests the condition number of SH fit of b=3000 shell directions at SH order 8

-   *Write multiple metrics of interest to standard output*::

        $ dirstat dwi.mif -output BN,BN-,BN+

    requests the mean, min and max nearest-neighour angles assuming a bipolar model.

Options
-------

-  **-output list** output selected metrics as a space-delimited list, suitable for use in scripts. This will produce one line of values per selected shell. Valid metrics are as specified in the description above.

DW shell selection options
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-shells b-values** specify one or more b-values to use during processing, as a comma-separated list of the desired approximate b-values (b-values are clustered to allow for small deviations). Note that some commands are incompatible with multiple b-values, and will report an error if more than one b-value is provided.  |br|
   WARNING: note that, even though the b=0 volumes are never referred to as shells in the literature, they still have to be explicitly included in the list of b-values as provided to the -shell option! Several algorithms which include the b=0 volumes in their computations may otherwise return an undesired result.

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-fslgrad bvecs bvals** Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

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



**Author:** J-Donald Tournier (jdtournier@gmail.com)

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


