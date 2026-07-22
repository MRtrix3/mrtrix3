.. _5ttgen_deep_atropos:

5ttgen deep_atropos
===================

Synopsis
--------

Generate the 5TT image based on a Deep Atropos segmentation or probabilities image

Usage
-----

::

    5ttgen deep_atropos input output [ options ]

-  *input*: The input Deep Atropos segmentation image
-  *output*: The output 5TT image

Description
-----------

This algorithm can accept the outputs of Deep Atropos in one of two forms. The "segmentation image" is a 3D image, of integer datatype, with indices mapping to discrete tissue classes as follows: 0: Background; 1: CSF; 2: Gray Matter; 3: White Matter; 4: Deep Gray Matter; 5: Brain Stem; 6: Cerebellum. The "probabilities images" are a set of seven 3D volumes, each corresponding to the posterior probability of one of the seven tissue classes above. These can be provided as input to this command by concatenating into a 4D image series with 7 volumes (the order of which must match that above).

The example usages provided in this help page, which include execution of Deep Atropos itself within a Python environment, require that "ants" and "antspynet" be installed via Python's "pip"; use of the "probability images" also requires that nibabel and numpy be installed.

Example usages
--------------

-   *To utilise the "segmentation" image*::

        $ python3 -c "import ants, antspynet; t1w = ants.image_read('T1w.nii.gz'); result = antspynet.deep_atropos(t1w); ants.image_write(result['segmentation_image'], 'segmentation.nii.gz')"; 5ttgen deep_atropos segmentation.nii.gz 5tt_segmentation.mif

    Because the input segmentation here is an integer image, where each voxel just contains an index corresponding to the maximal tissue class, the output 5TT image will not possess any fractional partial volumes; it will just contain the value 1.0 in whichever 5TT volume corresponds to the singular assigned tissue class.

-   *To utilise the "probability images"*::

        $ python3 -c "import ants, antspynet, nibabel, numpy; inpath = 'T1w.nii.gz'; t1w_ants = ants.image_read(inpath); t1w_nib = nibabel.load(inpath); result = antspynet.deep_atropos(t1w_ants); prob_maps = numpy.stack([numpy.array(img.numpy()) for img in result['probability_images']], axis=-1); nibabel.save(nibabel.Nifti1Image(prob_maps, t1w_nib.affine), 'probabilities.nii.gz')"; 5ttgen deep_atropos probabilities.nii.gz 5tt_probabilities.mif

    In this use case, the posterior probabilities of these tissue classes are interpreted as partial volume fractions and imported into the derivative 5TT image appropriately.

Options
-------

-  **-white_stem** Classify the brainstem as white matter

Options common to all 5ttgen algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-nocrop** Do NOT crop the resulting 5TT image to reduce its size (keep the same dimensions as the input image)

-  **-sgm_amyg_hipp** Represent the amygdalae and hippocampi as sub-cortical grey matter in the 5TT image

Standard options
^^^^^^^^^^^^^^^^

-  **-force** force overwrite of output files.

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading). (minimum: 0)

-  **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

Verbosity options
"""""""""""""""""

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages & debug input data.

*(these options are mutually exclusive; at most one may be specified)*


Additional standard options for Python scripts
""""""""""""""""""""""""""""""""""""""""""""""

-  **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

-  **-scratch /path/to/scratch/** manually specify an existing directory in which to generate the scratch directory.

-  **-continue ScratchDir LastFile** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

References
^^^^^^^^^^

Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. NeuroImage, 2012, 62, 1924-1938

N.J. Tustison, P.A. Cook, A.J. Holbrook, H.J. Johnson, J. Muschelli, G.A. Devenyi, J.T. Duda, S.R. Das, N.C. Cullen, D.L. Gillen, M.A. Yassa, J.R. Stone, J.C. Gee, and B.B. Avants. The ANTsX ecosystem for quantitative biological and medical imaging. Scientific Reports, 11(1):9068 (2021), pp. 1-13.

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Lucius S. Fekonja (lucius.fekonja[at]charite.de) and Robert E. Smith (robert.smith@florey.edu.au)

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

