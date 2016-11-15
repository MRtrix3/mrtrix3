.. _fod2fixel:

fod2fixel
===========

Synopsis
--------

::

    fod2fixel [ options ]  fod fixel_directory

-  *fod*: the input fod image.
-  *fixel_directory*: the output fixel directory

Description
-----------

use a fast-marching level-set method to segment fibre orientation distributions, and save parameters of interest as fixel images

Options
-------

-  **-mask image** only perform computation within the specified binary brain mask image.

Metric values for fixel-based sparse output images
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-afd image** output the total Apparent Fibre Density per fixel (integral of FOD lobe)

-  **-peak image** output the peak FOD amplitude per fixel

-  **-disp image** output a measure of dispersion per fixel as the ratio between FOD lobe integral and peak amplitude

FOD FMLS segmenter options
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-fmls_ratio_integral_to_neg value** threshold the ratio between the integral of a positive FOD lobe, and the integral of the largest negative lobe. Any lobe that fails to exceed the integral dictated by this ratio will be discarded. Default: 0.

-  **-fmls_ratio_peak_to_mean_neg value** threshold the ratio between the peak amplitude of a positive FOD lobe, and the mean peak amplitude of all negative lobes. Any lobe that fails to exceed the peak amplitude dictated by this ratio will be discarded. Default: 1.

-  **-fmls_peak_value value** threshold the raw peak amplitude of positive FOD lobes. Any lobe for which the peak amplitude is smaller than this threshold will be discarded. Default: 0.1.

-  **-fmls_no_thresholds** disable all FOD lobe thresholding; every lobe with a positive FOD amplitude will be retained.

-  **-fmls_peak_ratio_to_merge value** specify the amplitude ratio between a sample and the smallest peak amplitude of the adjoining lobes, above which the lobes will be merged. This is the relative amplitude between the smallest of two adjoining lobes, and the 'bridge' between the two lobes. A value of 1.0 will never merge two peaks into a single lobe; a value of 0.0 will always merge lobes unless they are bisected by a zero crossing. Default: 1.

-  **-nii** output the directions and index file in nii format (instead of the default mif)

-  **-dirpeak** define the fixel direction as the peak lobe direction as opposed to the lobe mean

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

References
^^^^^^^^^^

* Reference for the FOD segmentation method:Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. SIFT: Spherical-deconvolution informed filtering of tractograms. NeuroImage, 2013, 67, 298-312 (Appendix 2)

* Reference for Apparent Fibre Density:Raffelt, D.; Tournier, J.-D.; Rose, S.; Ridgway, G.R.; Henderson, R.; Crozier, S.; Salvado, O.; Connelly, A. Apparent Fibre Density: a novel measure for the analysis of diffusion-weighted magnetic resonance images.Neuroimage, 2012, 15;59(4), 3976-94.

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org

