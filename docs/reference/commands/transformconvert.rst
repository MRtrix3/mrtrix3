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
-  *operation*: the operation to perform, one of: |br|
   flirt_import, itk_import
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



**Author:** Max Pietsch (maximilian.pietsch@kcl.ac.uk)

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


