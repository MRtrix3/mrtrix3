.. _tckmap:

tckmap
===================

Synopsis
--------

Use track data as a form of contrast for producing a high-resolution image

Usage
--------

::

    tckmap [ options ]  tracks output

-  *tracks*: the input track file.
-  *output*: the output track-weighted image

Description
-----------

Note: if you run into limitations with RAM usage, make sure you output the results to a .mif file or .mih / .dat file pair - this will avoid the allocation of an additional buffer to store the output for write-out.

Options
-------

Options for the header of the output image
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-template image** an image file to be used as a template for the output (the output image will have the same transform and field of view).

-  **-vox size** provide either an isotropic voxel size (in mm), or comma-separated list of 3 voxel dimensions.

-  **-datatype spec** specify output image data type.

Options for the dimensionality of the output image
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-dec** perform track mapping in directionally-encoded colour (DEC) space

-  **-dixel path** map streamlines to dixels within each voxel; requires either a number of dixels (references an internal direction set), or a path to a text file containing a set of directions stored as azimuth/elevation pairs

-  **-tod lmax** generate a Track Orientation Distribution (TOD) in each voxel; need to specify the maximum spherical harmonic degree lmax to use when generating Apodised Point Spread Functions

Options for the TWI image contrast properties
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-contrast type** define the desired form of contrast for the output image |br|
   Options are: tdi, length, invlength, scalar_map, scalar_map_count, fod_amp, curvature, vector_file (default: tdi)

-  **-image image** provide the scalar image map for generating images with 'scalar_map' / 'scalar_map_count' contrast, or the spherical harmonics image for 'fod_amp' contrast

-  **-vector_file path** provide the vector data file for generating images with 'vector_file' contrast

-  **-stat_vox type** define the statistic for choosing the final voxel intensities for a given contrast type given the individual values from the tracks passing through each voxel.  |br|
   Options are: sum, min, mean, max (default: sum)

-  **-stat_tck type** define the statistic for choosing the contribution to be made by each streamline as a function of the samples taken along their lengths.  |br|
   Only has an effect for 'scalar_map', 'fod_amp' and 'curvature' contrast types.  |br|
   Options are: sum, min, mean, max, median, mean_nonzero, gaussian, ends_min, ends_mean, ends_max, ends_prod (default: mean)

-  **-fwhm_tck value** when using gaussian-smoothed per-track statistic, specify the desired full-width half-maximum of the Gaussian smoothing kernel (in mm)

-  **-map_zero** if a streamline has zero contribution based on the contrast & statistic, typically it is not mapped; use this option to still contribute to the map even if this is the case (these non-contributing voxels can then influence the mean value in each voxel of the map)

-  **-backtrack** when using -stat_tck ends_*, if the streamline endpoint is outside the FoV, backtrack along the streamline trajectory until an appropriate point is found

Options for the streamline-to-voxel mapping mechanism
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-upsample factor** upsample the tracks by some ratio using Hermite interpolation before mappping |br|
   (If omitted, an appropriate ratio will be determined automatically)

-  **-precise** use a more precise streamline mapping strategy, that accurately quantifies the length through each voxel (these lengths are then taken into account during TWI calculation)

-  **-ends_only** only map the streamline endpoints to the image

-  **-tck_weights_in path** specify a text scalar file containing the streamline weights

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

* For TDI or DEC TDI: |br|
  Calamante, F.; Tournier, J.-D.; Jackson, G. D. & Connelly, A. Track-density imaging (TDI): Super-resolution white matter imaging using whole-brain track-density mapping. NeuroImage, 2010, 53, 1233-1243

* If using -contrast length and -stat_vox mean: |br|
  Pannek, K.; Mathias, J. L.; Bigler, E. D.; Brown, G.; Taylor, J. D. & Rose, S. E. The average pathlength map: A diffusion MRI tractography-derived index for studying brain pathology. NeuroImage, 2011, 55, 133-141

* If using -dixel option with TDI contrast only: |br|
  Smith, R.E., Tournier, J-D., Calamante, F., Connelly, A. A novel paradigm for automated segmentation of very large whole-brain probabilistic tractography data sets. In proc. ISMRM, 2011, 19, 673

* If using -dixel option with any other contrast: |br|
  Pannek, K., Raffelt, D., Salvado, O., Rose, S. Incorporating directional information in diffusion tractography derived maps: angular track imaging (ATI). In Proc. ISMRM, 2012, 20, 1912

* If using -tod option: |br|
  Dhollander, T., Emsell, L., Van Hecke, W., Maes, F., Sunaert, S., Suetens, P. Track Orientation Density Imaging (TODI) and Track Orientation Distribution (TOD) based tractography. NeuroImage, 2014, 94, 312-336

* If using other contrasts / statistics: |br|
  Calamante, F.; Tournier, J.-D.; Smith, R. E. & Connelly, A. A generalised framework for super-resolution track-weighted imaging. NeuroImage, 2012, 59, 2494-2503

* If using -precise mapping option: |br|
  Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. SIFT: Spherical-deconvolution informed filtering of tractograms. NeuroImage, 2013, 67, 298-312 (Appendix 3)

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and J-Donald Tournier (jdtournier@gmail.com)

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


