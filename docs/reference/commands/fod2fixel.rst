.. _fod2fixel:

fod2fixel
===================

Synopsis
--------

Perform segmentation of continuous Fibre Orientation Distributions (FODs) to produce discrete fixels

Usage
--------

::

    fod2fixel [ options ]  fod fixel_directory

-  *fod*: the input fod image.
-  *fixel_directory*: the output fixel directory

Options
-------

Metric values for fixel-based sparse output images
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-afd image** output the total Apparent Fibre Density per fixel (integral of FOD lobe)

-  **-peak_amp image** output the amplitude of the FOD at the maximal peak per fixel

-  **-disp image** output a measure of dispersion per fixel as the ratio between FOD lobe integral and maximal peak amplitude

FOD FMLS segmenter options
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-fmls_integral value** threshold absolute numerical integral of positive FOD lobes. Any lobe for which the integral is smaller than this threshold will be discarded. Default: 0.

-  **-fmls_peak_value value** threshold peak amplitude of positive FOD lobes. Any lobe for which the maximal peak amplitude is smaller than this threshold will be discarded. Default: 0.1.

-  **-fmls_no_thresholds** disable all FOD lobe thresholding; every lobe where the FOD is positive will be retained.

-  **-fmls_lobe_merge_ratio value** Specify the ratio between a given FOD amplitude sample between two lobes, and the smallest peak amplitude of the adjacent lobes, above which those lobes will be merged. This is the amplitude of the FOD at the 'bridge' point between the two lobes, divided by the peak amplitude of the smaller of the two adjoining lobes. A value of 1.0 will never merge two lobes into one; a value of 0.0 will always merge lobes unless they are bisected by a zero-valued crossing. Default: 1.

Other options for fod2fixel
^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-mask image** only perform computation within the specified binary brain mask image.

-  **-maxnum number** maximum number of fixels to output for any particular voxel (default: no limit)

-  **-nii** output the directions and index file in nii format (instead of the default mif)

-  **-dirpeak** define the fixel direction as that of the lobe's maximal peak as opposed to its weighted mean direction (the default)

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

* Reference for the FOD segmentation method: |br|
  Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. SIFT: Spherical-deconvolution informed filtering of tractograms. NeuroImage, 2013, 67, 298-312 (Appendix 2)

* Reference for Apparent Fibre Density (AFD): |br|
  Raffelt, D.; Tournier, J.-D.; Rose, S.; Ridgway, G.R.; Henderson, R.; Crozier, S.; Salvado, O.; Connelly, A. Apparent Fibre Density: a novel measure for the analysis of diffusion-weighted magnetic resonance images.Neuroimage, 2012, 15;59(4), 3976-94

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

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


