.. _labelconfig:

labelconfig
===========

Synopsis
--------

::

    labelconfig [ options ]  path_in config_in image_out

-  *path_in*: the input image
-  *config_in*: the MRtrix connectome configuration file specifying desired nodes & indices
-  *image_out*: the output image

Description
-----------

prepare a parcellated image for connectome construction by modifying the image values; typically this involves making the parcellation intensities increment from 1 to coincide with rows and columns of a matrix. The configuration file passed as the second argument specifies the indices that should be assigned to different structures; examples of such configuration files are provided in src//connectome//config//

Options
-------

Options for importing information from parcellation lookup tables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-lut_basic path** get information from a basic lookup table consisting of index / name pairs

-  **-lut_freesurfer path** get information from a FreeSurfer lookup table (typically "FreeSurferColorLUT.txt")

-  **-lut_aal path** get information from the AAL lookup table (typically "ROI_MNI_V4.txt")

-  **-lut_itksnap path** get information from an ITK-SNAP lookup table (this includes the IIT atlas file "LUT_GM.txt")

-  **-spine image** provide a manually-defined segmentation of the base of the spine where the streamlines terminate, so that this can become a node in the connection matrix.

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



**Author:** Robert E. Smith (r.smith@brain.org.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

