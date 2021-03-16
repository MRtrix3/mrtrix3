.. _mrfilter:

mrfilter
===================

Synopsis
--------

Perform filtering operations on 3D / 4D MR images

Usage
--------

::

    mrfilter [ options ]  input filter output

-  *input*: the input image.
-  *filter*: the type of filter to be applied
-  *output*: the output image.

Description
-----------

The available filters are: boxblur, farid, fft, gaussian, laplacian3d, median, radialblur, sobel, sobelfeldman, sharpen, unsharpmask, zclean.

Each filter has its own unique set of optional parameters.

For 4D images, each 3D volume is processed independently.

Options
-------

Options for boxblur filter
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-extent size** specify extent of boxblur filtering kernel in voxels. This can be specified either as a single value to be used for all 3 axes, or as a comma-separated list of 3 values, one for each axis (default: 3x3x3).

Options for farid filter
^^^^^^^^^^^^^^^^^^^^^^^^

-  **-order value** specify order of derivative (default: 1st order i.e. gradient)

-  **-extent value** specify width of filter kernel (default: 1 + (2 x order)) (must be odd)

-  **-magnitude** output norm magnitude of filter result rather than 3 spatial components

Options for FFT filter
^^^^^^^^^^^^^^^^^^^^^^

-  **-axes list** the axes along which to apply the Fourier Transform. By default, the transform is applied along the three spatial axes. Provide as a comma-separate list of axis indices.

-  **-inverse** apply the inverse FFT

-  **-magnitude** output a magnitude image rather than a complex-valued image

-  **-centre_zero** re-arrange the FFT results so that the zero-frequency component appears in the centre of the image, rather than at the edges

Options for Gaussian filter
^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-fwhm value** set the Full-Width at Half-Maximum (FWHM) of the kernel in mm (default: 1 voxel)

-  **-radius value** set the radius in mm beyond which the kernel will be truncated (default: 3 x FWHM)

Options for median filter
^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-extent size** specify extent of median filtering neighbourhood in voxels. This can be specified either as a single value to be used for all 3 axes, or as a comma-separated list of 3 values, one for each axis (default: 3x3x3).

Options for radialblur filter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-radius value** specify radius of blurring kernel in mm. (Default: 2 voxels along axis with largest voxel spacing)

Options for sobel and sobelfeldman filters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-magnitude** output norm magnitude of filter result rather than 3 spatial components

Options for sharpen filter
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-strength value** specify the strength of the sharpening kernel. (default = 1.0)

Options for UnsharpMask filter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-fwhm value** set the Full-Width at Half-Maximum (FWHM) of the smoothing kernel in mm (default: 2 voxels)

-  **-strength value** set the strength of the edge enhancement (default: 1.0)

Options for zclean filter
^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-zupper num** define high intensity outliers: default: 2.5

-  **-zlower num** define low intensity outliers: default: 2.5

-  **-bridge num** number of voxels to gap to fill holes in mask: default: 4

-  **-maskin image** initial mask that defines the maximum spatial extent and the region from which to sample the intensity range.

-  **-maskout image** Output a refined mask based on a spatially coherent region with normal intensity range.

Stride options
^^^^^^^^^^^^^^

-  **-strides spec** specify the strides of the output data in memory; either as a comma-separated list of (signed) integers, or as a template image from which the strides shall be extracted and used. The actual strides produced will depend on whether the output image format can support it.

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



**Author:** Robert E. Smith (robert.smith@florey.edu.au), David Raffelt (david.raffelt@florey.edu.au) and J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2019 the MRtrix3 contributors.

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


