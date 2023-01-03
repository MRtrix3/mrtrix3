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

The available filters are: fft, gradient, median, smooth, normalise, zclean.

Each filter has its own unique set of optional parameters.

For 4D images, each 3D volume is processed independently.

Options
-------

Options for FFT filter
^^^^^^^^^^^^^^^^^^^^^^

-  **-axes list** the axes along which to apply the Fourier Transform. By default, the transform is applied along the three spatial axes. Provide as a comma-separate list of axis indices.

-  **-inverse** apply the inverse FFT

-  **-magnitude** output a magnitude image rather than a complex-valued image

-  **-centre_zero** re-arrange the FFT results so that the zero-frequency component appears in the centre of the image, rather than at the edges

Options for gradient filter
^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-stdev sigma** the standard deviation of the Gaussian kernel used to smooth the input image (in mm). The image is smoothed to reduced large spurious gradients caused by noise. Use this option to override the default stdev of 1 voxel. This can be specified either as a single value to be used for all 3 axes, or as a comma-separated list of 3 values, one for each axis.

-  **-magnitude** output the gradient magnitude, rather than the default x,y,z components

-  **-scanner** define the gradient with respect to the scanner coordinate frame of reference.

Options for median filter
^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-extent size** specify extent of median filtering neighbourhood in voxels. This can be specified either as a single value to be used for all 3 axes, or as a comma-separated list of 3 values, one for each axis (default: 3x3x3).

Options for normalisation filter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-extent size** specify extent of normalisation filtering neighbourhood in voxels. This can be specified either as a single value to be used for all 3 axes, or as a comma-separated list of 3 values, one for each axis (default: 3x3x3).

Options for smooth filter
^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-stdev mm** apply Gaussian smoothing with the specified standard deviation. The standard deviation is defined in mm (Default 1 voxel). This can be specified either as a single value to be used for all axes, or as a comma-separated list of the stdev for each axis.

-  **-fwhm mm** apply Gaussian smoothing with the specified full-width half maximum. The FWHM is defined in mm (Default 1 voxel * 2.3548). This can be specified either as a single value to be used for all axes, or as a comma-separated list of the FWHM for each axis.

-  **-extent voxels** specify the extent (width) of kernel size in voxels. This can be specified either as a single value to be used for all axes, or as a comma-separated list of the extent for each axis. The default extent is 2 * ceil(2.5 * stdev / voxel_size) - 1.

Options for zclean filter
^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-zupper num** define high intensity outliers: default: 2.5

-  **-zlower num** define low intensity outliers: default: 2.5

-  **-bridge num** number of voxels to gap to fill holes in mask: default: 4

-  **-maskin image** initial mask that defines the maximum spatial extent and the region from which to smaple the intensity range.

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


