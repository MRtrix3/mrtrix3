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

The program currently supports MRtrix .tck files (input/output), ascii text files (input/output), VTK polydata files (input/output), QFib lossy compressed .qfib files (input/output), and RenderMan RIB (export only).

The QFib format (Mercier et al.) stores each streamline as its first two vertices plus a sequence of quantized unit tangents. It is lossy, requires the input to be of constant step size (resample beforehand with "tckresample -step_size" otherwise), and stores geometry only: per-streamline weights and dps/dpv sidecar data are discarded.

Some tractography file formats (the TrackVis ".trk" format and the TRX format) can embed per-streamline (dps) and per-vertex (dpv) sidecar data within the tractogram dataset itself. The -extract, -insert, -rename, -remove and -convert options manipulate this embedded data during conversion. Each takes a leading "dps" or "dpv" argument to disambiguate the two, since a per-streamline and a per-vertex field may legitimately share the same name. Per-streamline data is exchanged with standalone numerical files (text, ".csv" or ".npy"); per-vertex data with track scalar (".tsf") files. When a ".tsf" is produced from extracted per-vertex data, a matching "timestamp" key-value is recorded on both it and the output tractogram so the pair passes the track-scalar validation checks. Fields are always referenced by string name, never by index.

By default vertex positions are read and written in MRtrix3 real (scanner) space. The -input_is_voxelspace and -input_is_imagespace options instead interpret the vertex positions of the input tractogram as voxel coordinates, or as image coordinates (in mm), of the provided reference image, converting them to real space for internal processing and output; the two are mutually exclusive. The -output_as_voxelspace option encodes the vertex positions of the output tractogram as voxel coordinates of the provided reference image rather than in real space; this requires an output format able to embed the corresponding voxel-to-real-space transform within its header (for example the TRX format), and raises an error otherwise.

Example usages
--------------

-   *Writing multiple ASCII files, one per streamline*::

        $ tckconvert input.tck output-[].txt

    By using the multi-file numbering syntax, where square brackets denote the position of the numbering for the files, this example will produce files named output-0000.txt, output-0001.txt, output-0002.txt, ...

Options
-------

Options to specify the coordinate space of the input and/or output vertex positions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-input_is_voxelspace reference** interpret the input tractogram vertex positions as voxel coordinates of this reference image

-  **-input_is_imagespace reference** interpret the input tractogram vertex positions as image coordinates (in mm) of this reference image

-  **-output_as_voxelspace reference** store the output tractogram vertex positions as voxel coordinates of this reference image

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

-  **-ascii** write an ASCII VTK file (binary by default)

Options specific to ZFIB writer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-zfib_max_error value** the worst-case compression error in mm for lossy .zfib output (default: 0.5)

Options specific to the QFib writer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-qfib_bits depth** the per-direction quantization bit depth for lossy .qfib output, either 8 or 16 (default: 16)

-  **-qfib_max_angle angle** the maximum streamline deviation angle in degrees for lossy .qfib output; defaults to the max_angle property of the input, else 90

Options for manipulating embedded sidecar data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-extract type name file** *(multiple uses permitted)* extract an embedded sidecar field, referenced by name, to a standalone file

-  **-insert type name file** *(multiple uses permitted)* embed a new sidecar field, read from a standalone file, into the output

-  **-rename type old new** *(multiple uses permitted)* rename an embedded sidecar field

-  **-remove type name** *(multiple uses permitted)* remove an embedded sidecar field

-  **-convert type name datatype** *(multiple uses permitted)* change the on-disk datatype of an embedded sidecar field

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages & debug input data.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Daan Christiaens (daan.christiaens@kcl.ac.uk) and J-Donald Tournier (jdtournier@gmail.com) and Philip Broser (philip.broser@me.com) and Daniel Blezek (daniel.blezek@gmail.com)

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


