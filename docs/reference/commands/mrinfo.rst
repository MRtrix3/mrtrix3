.. _mrinfo:

mrinfo
===========

Synopsis
--------

::

    mrinfo [ options ]  image [ image ... ]

-  *image*: the input image(s).

Description
-----------

display header information, or extract specific information from the header.

By default, all information contained in each image header will be printed to the console in a reader-friendly format.

Alternatively, command-line options may be used to extract specific details from the header(s); these are printed to the console in a format more appropriate for scripting purposes or piping to file. If multiple options and/or images are provided, the requested header fields will be printed in the order in which they appear in the help page, with all requested details from each input image in sequence printed before the next image is processed.

The command can also write the diffusion gradient table from a single input image to file; either in the MRtrix or FSL format (bvecs/bvals file pair; includes appropriate diffusion gradient vector reorientation)

Options
-------

-  **-format** image file format

-  **-ndim** number of image dimensions

-  **-size** image size along each axis

-  **-vox** voxel size along each image dimension

-  **-datatype** data type used for image data storage

-  **-stride** data strides i.e. order and direction of axes data layout

-  **-offset** image intensity offset

-  **-multiplier** image intensity multiplier

-  **-transform** the voxel to image transformation

-  **-norealign** do not realign transform to near-default RAS coordinate system (the default behaviour on image load). This is useful to inspect the transform and strides as they are actually stored in the header, rather than as MRtrix interprets them.

-  **-property key** any text properties embedded in the image header under the specified key (use 'all' to list all keys found)

-  **-json_export file** export header key/value entries to a JSON file

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad encoding** specify the diffusion-weighted gradient scheme used in the acquisition. The program will normally attempt to use the encoding stored in the image header. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2.

-  **-fslgrad bvecs bvals** specify the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format.

-  **-bvalue_scaling mode** specifies whether the b-values should be scaled by the square of the corresponding DW gradient norm, as often required for multi-shell or DSI DW acquisition schemes. The default action can also be set in the MRtrix config file, under the BValueScaling entry. Valid choices are yes/no, true/false, 0/1 (default: true).

-  **-raw_dwgrad** do not modify the gradient table from what was found in the image headers. This skips the validation steps normally performed within MRtrix applications (i.e. do not verify that the number of entries in the gradient table matches the number of volumes in the image, do not scale b-values by gradient norms, do not normalise gradient vectors)

DW gradient table export options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-export_grad_mrtrix path** export the diffusion-weighted gradient table to file in MRtrix format

-  **-export_grad_fsl bvecs_path bvals_path** export the diffusion-weighted gradient table to files in FSL (bvecs / bvals) format

-  **-dwgrad** the diffusion-weighting gradient table, as stored in the header (i.e. without any interpretation, scaling of b-values, or normalisation of gradient vectors)

-  **-shells** list the average b-value of each shell

-  **-shellcounts** list the number of volumes in each shell

Options for exporting phase-encode tables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-export_pe_table file** export phase-encoding table to file

-  **-export_pe_eddy config indices** export phase-encoding information to an EDDY-style config / index file pair

-  **-petable** print the phase encoding table

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



**Author:** J-Donald Tournier (d.tournier@brain.org.au) and Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

