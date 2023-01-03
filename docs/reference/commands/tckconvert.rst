.. _tckconvert:

tckconvert
===================

Synopsis
--------

Convert between different track file formats

Usage
--------

::

    tckconvert [ options ]  input output

-  *input*: the input track file.
-  *output*: the output track file.

Description
-----------

The program currently supports MRtrix .tck files (input/output), ascii text files (input/output), VTK polydata files (input/output), and RenderMan RIB (export only).

Note that ascii files will be stored with one streamline per numbered file. To support this, the command will use the multi-file numbering syntax, where square brackets denote the position of the numbering for the files, for example:

$ tckconvert input.tck output-'[]'.txt

will produce files named output-0000.txt, output-0001.txt, output-0002.txt, ...

Options
-------

-  **-scanner2voxel reference** if specified, the properties of this image will be used to convert track point positions from real (scanner) coordinates into voxel coordinates.

-  **-scanner2image reference** if specified, the properties of this image will be used to convert track point positions from real (scanner) coordinates into image coordinates (in mm).

-  **-voxel2scanner reference** if specified, the properties of this image will be used to convert track point positions from voxel coordinates into real (scanner) coordinates.

-  **-image2scanner reference** if specified, the properties of this image will be used to convert track point positions from image coordinates (in mm) into real (scanner) coordinates.

Options specific to PLY writer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-sides sides** number of sides for streamlines

-  **-increment increment** generate streamline points at every (increment) points

Options specific to RIB writer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-dec** add DEC as a primvar

Options for both PLY and RIB writer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-radius radius** radius of the streamlines

Options specific to VTK writer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-ascii** write an ASCII VTK file (this is the default)

-  **-binary** write a binary VTK file

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



**Author:** Daan Christiaens (daan.christiaens@kcl.ac.uk), J-Donald Tournier (jdtournier@gmail.com), Philip Broser (philip.broser@me.com), Daniel Blezek (daniel.blezek@gmail.com).

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


