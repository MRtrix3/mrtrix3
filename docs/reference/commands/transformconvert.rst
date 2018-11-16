.. _transformconvert:

transformconvert
===================

Synopsis
--------

Convert linear transformation matrices

Usage
--------

::

    transformconvert [ options ]  input [ input ... ] operation output

-  *input*: the input(s) for the specified operation
-  *operation*: the operation to perform, one of:flirt_import, itk_import
-  *output*: the output transformation matrix.

Description
-----------

This command allows to convert transformation matrices provided by other registration softwares to a format usable in MRtrix3. Example usages are provided below.

Example usages
--------------

-   *Convert a transformation matrix produced by FSL's flirt command into a format usable by MRtrix3*::

        $ transformconvert transform_flirt.mat flirt_in.nii flirt_ref.nii flirt_import transform_mrtrix.txt

    The two images provided as inputs for this operation must be in the correct order: first the image that was provided to flirt via the -in option, second the image that was provided to flirt via the -ref option.

-   *Convert a plain text transformation matrix file produced by ITK's affine registration (e.g. ANTS, Slicer) into a format usable by MRtrix3*::

        $ transformconvert transform_itk.txt itk_import transform_mrtrix.txt

Options
-------

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



**Author:** Max Pietsch (maximilian.pietsch@kcl.ac.uk)

**Copyright:** Copyright (c) 2008-2018 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/

MRtrix3 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/


