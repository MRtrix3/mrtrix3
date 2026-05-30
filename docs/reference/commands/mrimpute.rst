.. _mrimpute:

mrimpute
===================
Synopsis
--------

Impute predicted intensities into invalid image voxels

Usage
--------

::

    mrimpute [ options ]  input output

-  *input*: the input image
-  *output*: the output image

Description
-----------

This command fills "invalid" voxels in an image with intensities predicted from the surrounding valid data. By default, voxels with non-finite values (NaN or Inf) are imputed; the -mask option flags additional voxels to be imputed. For images with more than three axes, each 3D volume is processed independently.

The set of voxels to be imputed is serialised into the unknowns of a linear system that is solved per 3D volume. The available algorithms are derived from the 2D MatLab module Inpaint_nans (generalised here to 3D), together with two "isotropic" methods that solve the same partial differential equations over a spherically-symmetric finite-difference stencil:

laplacian: solve the Laplace (del-squared) equation as an overdetermined least-squares system (Inpaint_nans method 0).

laplaciansq: solve the same Laplace equation as a square system, with exactly one equation per imputed voxel (Inpaint_nans method 2).

biharmonic: solve the biharmonic (del-to-the-fourth) equation (Inpaint_nans method 3).

spring: constrain each imputed voxel toward equality with its neighbours (Inpaint_nans method 4); yields constant extrapolation.

isotropic2 / isotropic4: as for laplacian / biharmonic respectively, but assembled from a 13-direction spherical-harmonic-weighted stencil for improved rotational invariance.

Inpaint_nans methods 1 (redundant with method 0) and 5 (an author-discouraged neighbour average) are intentionally omitted. The linear solver is selected automatically per method (dense QR for the least-squares methods; dense LU for the square method).

The imputation system is dense and scoped to each 3D volume; this is efficient for typical hole counts, but very large contiguous regions to be imputed will produce a large dense system.

Options
-------

-  **-mask image** a bitwise mask image flagging additional voxels to impute (beyond the non-finite voxels imputed by default)

-  **-method name** the imputation algorithm to use (default: laplacian); one of: laplacian, laplaciansq, biharmonic, spring, isotropic2, isotropic4

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



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

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


