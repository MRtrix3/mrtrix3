.. _amp2sh:

amp2sh
===================

Synopsis
--------

Convert a set of amplitudes (defined along a set of corresponding directions) to their spherical harmonic representation

Usage
--------

::

    amp2sh [ options ]  amp SH

-  *amp*: the input amplitude image.
-  *SH*: the output spherical harmonics coefficients image.

Description
-----------

The spherical harmonic decomposition is calculated by least-squares linear fitting to the amplitude data.

The directions can be defined either as a DW gradient scheme (for example to compute the SH representation of the DW signal) or a set of [az el] pairs as output by the dirgen command. The DW gradient scheme or direction set can be supplied within the input image header or using the -gradient or -directions option. Note that if a direction set and DW gradient scheme can be found, the direction set will be used by default.

The spherical harmonic coefficients are stored as follows. First, since the signal attenuation profile is real, it has conjugate symmetry, i.e. Y(l,-m) = Y(l,m)* (where * denotes the complex conjugate). Second, the diffusion profile should be antipodally symmetric (i.e. S(x) = S(-x)), implying that all odd l components should be zero. Therefore, only the even elements are computed. Note that the spherical harmonics equations used here differ slightly from those conventionally used, in that the (-1)^m factor has been omitted. This should be taken into account in all subsequent calculations. Each volume in the output image corresponds to a different spherical harmonic component. Each volume will correspond to the following: volume 0: l = 0, m = 0 ; volume 1: l = 2, m = -2 (imaginary part of m=2 SH) ; volume 2: l = 2, m = -1 (imaginary part of m=1 SH) ; volume 3: l = 2, m = 0 ; volume 4: l = 2, m = 1 (real part of m=1 SH) ; volume 5: l = 2, m = 2 (real part of m=2 SH) ; etc...

Options
-------

-  **-lmax order** set the maximum harmonic order for the output series. By default, the program will use the highest possible lmax given the number of diffusion-weighted images, up to a maximum of 8.

-  **-normalise** normalise the DW signal to the b=0 image

-  **-directions file** the directions corresponding to the input amplitude image used to sample AFD. By default this option is not required providing the direction set is supplied in the amplitude image. This should be supplied as a list of directions [az el], as generated using the dirgen command

-  **-rician noise** correct for Rician noise induced bias, using noise map supplied

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-fslgrad bvecs bvals** Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-bvalue_scaling mode** specifies whether the b-values should be scaled by the square of the corresponding DW gradient norm, as often required for multi-shell or DSI DW acquisition schemes. The default action can also be set in the MRtrix config file, under the BValueScaling entry. Valid choices are yes/no, true/false, 0/1 (default: true).

DW shell selection options
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-shells list** specify one or more diffusion-weighted gradient shells to use during processing, as a comma-separated list of the desired approximate b-values. Note that some commands are incompatible with multiple shells, and will throw an error if more than one b-value is provided.

Stride options
^^^^^^^^^^^^^^

-  **-strides spec** specify the strides of the output data in memory, as a comma-separated list. The actual strides produced will depend on whether the output image format can support it.

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


