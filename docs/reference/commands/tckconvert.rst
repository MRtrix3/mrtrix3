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

$ tckconvert input.tck output-[].txt

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

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------



**Author:** Daan Christiaens (daan.christiaens@kcl.ac.uk), J-Donald Tournier (jdtournier@gmail.com), Philip Broser (philip.broser@me.com), Daniel Blezek (daniel.blezek@gmail.com).

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


