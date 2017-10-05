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

Options
-------

Output options
^^^^^^^^^^^^^^

-  **-bipolar** output statistics for bipolar electrostatic repulsion model

-  **-unipolar** output statistics for unipolar electrostatic repulsion model

-  **-shfit** output statistics for spherical harmonics fit

-  **-symmetry** output measure of symmetry of spherical coverage (as given by the norm of mean direction vector). This is important to ensure minimal bias due to eddy-currents.

-  **-nearest_neighour** output nearest-neighbour angle statistics

-  **-energy** output energy statistics

-  **-total** output total of statistic (affects -energy only)

-  **-mean** output mean of statistic (affects -nearest_neighbour or -energy only)

-  **-range** output range of statistic (affects -nearest_neighbour or -energy only)

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-fslgrad bvecs bvals** Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-bvalue_scaling mode** specifies whether the b-values should be scaled by the square of the corresponding DW gradient norm, as often required for multi-shell or DSI DW acquisition schemes. The default action can also be set in the MRtrix config file, under the BValueScaling entry. Valid choices are yes/no, true/false, 0/1 (default: true).

DW shell selection options
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-shells list** specify one or more diffusion-weighted gradient shells to use during processing, as a comma-separated list of the desired approximate b-values. Note that some commands are incompatible with multiple shells, and will throw an error if more than one b-value is provided.

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



**Author:** J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


