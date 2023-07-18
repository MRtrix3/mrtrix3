.. _mrinfo:

mrinfo
===================

Synopsis
--------

Display image header information, or extract specific information from the header

Usage
--------

::

    mrinfo [ options ]  image [ image ... ]

-  *image*: the input image(s).

Description
-----------

By default, all information contained in each image header will be printed to the console in a reader-friendly format.

Alternatively, command-line options may be used to extract specific details from the header(s); these are printed to the console in a format more appropriate for scripting purposes or piping to file. If multiple options and/or images are provided, the requested header fields will be printed in the order in which they appear in the help page, with all requested details from each input image in sequence printed before the next image is processed.

The command can also write the diffusion gradient table from a single input image to file; either in the MRtrix or FSL format (bvecs/bvals file pair; includes appropriate diffusion gradient vector reorientation)

The -dwgrad, -export_* and -shell_* options provide (information about) the diffusion weighting gradient table after it has been processed by the MRtrix3 back-end (vectors normalised, b-values scaled by the square of the vector norm, depending on the -bvalue_scaling option). To see the raw gradient table information as stored in the image header, i.e. without MRtrix3 back-end processing, use "-property dw_scheme".

The -bvalue_scaling option controls an aspect of the import of diffusion gradient tables. When the input diffusion-weighting direction vectors have norms that differ substantially from unity, the b-values will be scaled by the square of their corresponding vector norm (this is how multi-shell acquisitions are frequently achieved on scanner platforms). However in some rare instances, the b-values may be correct, despite the vectors not being of unit norm (or conversely, the b-values may need to be rescaled even though the vectors are close to unit norm). This option allows the user to control this operation and override MRrtix3's automatic detection.

Options
-------

-  **-all** print all properties, rather than the first and last 2 of each.

-  **-name** print the file system path of the image

-  **-format** image file format

-  **-ndim** number of image dimensions

-  **-size** image size along each axis

-  **-spacing** voxel spacing along each image dimension

-  **-datatype** data type used for image data storage

-  **-strides** data strides i.e. order and direction of axes data layout

-  **-offset** image intensity offset

-  **-multiplier** image intensity multiplier

-  **-transform** the transformation from image coordinates [mm] to scanner / real world coordinates [mm]

Options for exporting image header fields
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-property key** *(multiple uses permitted)* any text properties embedded in the image header under the specified key (use 'all' to list all keys found)

-  **-json_keyval file** export header key/value entries to a JSON file

-  **-json_all file** export all header contents to a JSON file

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-fslgrad bvecs bvals** Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-bvalue_scaling mode** enable or disable scaling of diffusion b-values by the square of the corresponding DW gradient norm (see Desciption). Valid choices are yes/no, true/false, 0/1 (default: automatic).

DW gradient table export options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-export_grad_mrtrix path** export the diffusion-weighted gradient table to file in MRtrix format

-  **-export_grad_fsl bvecs_path bvals_path** export the diffusion-weighted gradient table to files in FSL (bvecs / bvals) format

-  **-dwgrad** the diffusion-weighting gradient table, as interpreted by MRtrix3

-  **-shell_bvalues** list the average b-value of each shell

-  **-shell_sizes** list the number of volumes in each shell

-  **-shell_indices** list the image volumes attributed to each b-value shell

Options for exporting phase-encode tables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-export_pe_table file** export phase-encoding table to file

-  **-export_pe_eddy config indices** export phase-encoding information to an EDDY-style config / index file pair

-  **-petable** print the phase encoding table

Handling of piped images
^^^^^^^^^^^^^^^^^^^^^^^^

-  **-nodelete** don't delete temporary images or images passed to mrinfo via Unix pipes

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



**Author:** J-Donald Tournier (d.tournier@brain.org.au) and Robert E. Smith (robert.smith@florey.edu.au)

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


