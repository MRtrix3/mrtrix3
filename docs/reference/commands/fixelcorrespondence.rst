.. _fixelcorrespondence:

fixelcorrespondence
===================

Synopsis
--------

Establish correpondence between two fixel datasets

Usage
--------

::

    fixelcorrespondence [ options ]  source_density target_density output

-  *source_density*: the input source fixel data file corresponding to the FD or FDC metric
-  *target_density*: the input target fixel data file corresponding to the FD or FDC metric
-  *output*: the name of the output .npz file encoding the fixel correspondence

Description
-----------

It is assumed that the source image has already been spatially normalised and is defined on the same voxel grid as the target. One would typically also want to have performed a reorientation of fibre information to reflect this spatial normalisation prior to invoking this command, as this would be expected to improve fibre orientation correspondence across datasets.

The output of the command is a .npz file (uncompressed ZIP archive) encoding how data from source fixels should be remapped  in order to express those data in target fixel space. This information would typically then be utilised by command fixel2fixel  to project some quantitative parameter from the source fixel dataset to the target fixels.

Multiple algorithms are provided; a brief description of each of these is provided below.

"all2all": This algorithm is defined for debugging / demonstrative purposes only. It assigns all source fixels to all target fixels, and is therefore not appropriate for practical use.

"legacy": This algorithm duplicates the behaviour of the fixelcorrespondence command in MRtrix versions 3.0.x. and earlier. It determines, for every target fixel, the nearest source fixel, and assigns that source fixel to the target fixel with a weight of 1.0, as long as the angle between them is less than some threshold. Note that if multiple target fixels select the same source fixel, the entirety of the data from that source fixel is projected to each of those target fixels independently.

"ismrm2018": This is a combinatorial algorithm, for which the algorithm and cost function are described in the relevant reference (Smith et al., 2018).

"in2023": This is a combinatorial algorithm, for which the combinatorial algorithm utilised is described in reference (Smith et al., 2018), but an alternative cost function is proposed (publication pending).

"pot": This is a combinatorial algorithm using a partial-optimal-transport-inspired cost function. Matched fibre density between subject and template fixels is "transported" at a cost determined by directional misalignment, while surplus density on either side is created or destroyed at unit cost; subject or template fixels with no correspondence are penalised by their density. Mapping topology (multiple subject fixels merged into one template fixel, or one subject fixel split across multiple template fixels) is penalised linearly with weight controlled by the "gamma" parameter, and the angular sensitivity is controlled by exponent "p".

"transport": This is a combinatorial algorithm in which each subject fixel's fibre density is treated as mass to be transported to the template fixel(s) it is assigned to, paying an angular cost; subject mass that cannot be placed within a threshold angle is left unmatched. Crucially, template fibre density enters the cost only through template orientation, never as a matching target, so the optimal mapping does not shrink subject density toward the template and between-subject contrast is preserved.

"transportdisp": As for "transport", but each remapped fixel's assembled mass is scored by the alignment of its mean direction together with an explicit penalty on the angular dispersion of the merged subject fixels. This more strongly rewards merging fixels that straddle a template direction while penalising the conflation of widely-separated populations.

"agreement": A combinatorial algorithm evolving the "ismrm2018" cost. Density disagreement between remapped and template fixels is gated by angular misalignment (so it is ignored where the geometry is good) and saturates beyond a contrast-protection scale "sigma", so that genuinely differing subject densities are not dragged toward the template.

"transportguard": As for "transport", but with an additional one-sided penalty that fires only when a remapped fixel accumulates substantially more mass than the corresponding template fixel plausibly holds, suppressing non-physical pile-ups from over-merging without otherwise affecting density contrast.

"maskoverlap": This is a combinatorial algorithm that scores correspondence by the geometric overlap between the dixel masks of the remapped subject fixels and those of the template fixels, using no FOD amplitude information. It reuses the same cost skeleton as "pot" (matched density transported at a cost, surplus density created or destroyed at unit cost, and a linear parsimony penalty on merging or splitting fixels controlled by "gamma"), but the directional misalignment term is replaced by the fraction of each remapped subject lobe that is not explained by its paired template lobe. Contributions from directions shared between multiple fixels are down-weighted so that they are not double-counted. Both the source and target fixel directories must carry a per-fixel dixel-mask file (as exported by fod2fixel).

Options
-------

-  **-algorithm choice** the algorithm to use when establishing fixel correspondence; options are: all2all, legacy, ismrm2018, pot, rs2023, transport, transportdisp, agreement, transportguard, maskoverlap (default: pot)

-  **-remapped path** export the remapped source fixels to a new fixel directory

Options specific to algorithm "legacy"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-angle value** maximum angle within which a corresponding fixel may be selected, in degrees (default: 45)

Options applicable to all combinatorial-based algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-max_origins value** maximal number of origin source fixels for an individual target fixel (default: 3)

-  **-max_objectives value** maximal number of objective target fixels for an individual source fixel (default: 3)

-  **-cost path** export a 3D image containing the optimal value of the relevant cost function in each voxel

Options specific to algorithm "pot"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-pot_complexity value** weight "gamma" applied to the linear penalty for merging multiple subject fixels into one template fixel or splitting one subject fixel across multiple template fixels (default: 0.5)

Options specific to algorithm "rs2023"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-rs2023_constants alpha beta** set values for the two constants that modulate the influence of different cost function terms in the RS2023 expression

Options specific to algorithms "transport", "transportdisp" and "transportguard"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-transport_kernel choice** the angular cost kernel a(theta): "tan" or "tan2" (default: tan2)

-  **-transport_angle value** the threshold angle theta* (degrees) beyond which subject fibre density is left unplaced rather than transported to a poorly-aligned template fixel (default: 45)

-  **-transport_complexity value** weight "gamma" applied to the linear parsimony penalty on fixel merging and splitting (default: 0.5)

Options specific to algorithm "transportdisp" (in addition to those shared with "transport")
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-transportdisp_dispersion value** weight "lambda" applied to the within-fixel angular dispersion penalty (default: 1)

Options specific to algorithm "agreement"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-agreement_kernel choice** the angular cost kernel a(theta): "tan" or "tan2" (default: tan2)

-  **-agreement_sigma value** the density-contrast protection scale "sigma" beyond which density disagreement saturates (default: 1)

-  **-agreement_complexity value** weight "beta" applied to the squared parsimony penalty on fixel merging and splitting (default: 0.100000001)

Options specific to algorithm "transportguard" (in addition to those shared with "transport")
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-transportguard_overexplain mu rho** set the weight "mu" and density ratio threshold "rho" of the one-sided over-explanation guard (defaults: 1 2)

Options specific to algorithm "maskoverlap"
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-maskoverlap_complexity value** weight "gamma" applied to the linear penalty for merging multiple subject fixels into one template fixel or splitting one subject fixel across multiple template fixels (default: 0.5)

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

* If using -algorithm ismrm2018 or -algorithm rs2023: Smith, R.E.; Connelly, A. Mitigating the effects of imperfect fixel correspondence in Fixel-Based Analysis. In Proc ISMRM 2018: 456.

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


