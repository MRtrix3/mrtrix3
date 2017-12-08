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

Specific metrics can also be queried independently via the "-output" option, using these shorthands: U/B for unipolar/bipolar model, E/N for energy and nearest-neighbour respectively, t/-/+ for total/min/max respectively (mean implied otherwise); SHn for condition number of SH fit at order n (with n an even integer); ASYM for asymmetry index (norm of mean direction vector); and N for the number of directions. For example:

-output BN,BN-,BN+   requests the mean, min and max nearest-neighour angles assuming a bipolar model.

-output UE,SH8,SYM   requests the mean unipolar electrostatic energy, condition number of SH fit at order 8, and the asymmetry index.

Options
-------

-  **-output list** output selected metrics as a space-delimited list, suitable for use in scripts. This will produce one line of values per selected shell. Valid metrics are as specified in the description above.

DW shell selection options
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-shells list** specify one or more diffusion-weighted gradient shells to use during processing, as a comma-separated list of the desired approximate b-values. Note that some commands are incompatible with multiple shells, and will throw an error if more than one b-value is provided.

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-fslgrad bvecs bvals** Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-bvalue_scaling mode** specifies whether the b-values should be scaled by the square of the corresponding DW gradient norm, as often required for multi-shell or DSI DW acquisition schemes. The default action can also be set in the MRtrix config file, under the BValueScaling entry. Valid choices are yes/no, true/false, 0/1 (default: true).

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



**Author:** J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


