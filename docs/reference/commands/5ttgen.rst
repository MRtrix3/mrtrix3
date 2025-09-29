.. _5ttgen:

5ttgen
======

Synopsis
--------

Generate a 5TT image suitable for ACT

Usage
-----

::

    5ttgen algorithm [ options ] ...

-  *algorithm*: Select the algorithm to be used; additional details and options become available once an algorithm is nominated. Options are: deep_atropos, freesurfer, fsl, gif, hsvs

Description
-----------

5ttgen acts as a "master" script for generating a five-tissue-type (5TT) segmented tissue image suitable for use in Anatomically-Constrained Tractography (ACT). A range of different algorithms are available for completing this task. When using this script, the name of the algorithm to be used must appear as the first argument on the command-line after "5ttgen". The subsequent compulsory arguments and options available depend on the particular algorithm being invoked.

Each algorithm available also has its own help page, including necessary references; e.g. to see the help page of the "fsl" algorithm, type "5ttgen fsl".

Options
-------

Options common to all 5ttgen algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocrop** Do NOT crop the resulting 5TT image to reduce its size (keep the same dimensions as the input image)

- **-sgm_amyg_hipp** Represent the amygdalae and hippocampi as sub-cortical grey matter in the 5TT image

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify an existing directory in which to generate the scratch directory.

- **-continue ScratchDir LastFile** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

Standard options
^^^^^^^^^^^^^^^^

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages.

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

- **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

* Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. NeuroImage, 2012, 62, 1924-1938

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2025 the MRtrix3 contributors.

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

- **-white_stem** Classify the brainstem as white matter

Options common to all 5ttgen algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocrop** Do NOT crop the resulting 5TT image to reduce its size (keep the same dimensions as the input image)

- **-sgm_amyg_hipp** Represent the amygdalae and hippocampi as sub-cortical grey matter in the 5TT image

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify an existing directory in which to generate the scratch directory.

- **-continue ScratchDir LastFile** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

Standard options
^^^^^^^^^^^^^^^^

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages.

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

- **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

* Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. NeuroImage, 2012, 62, 1924-1938

* N.J. Tustison, P.A. Cook, A.J. Holbrook, H.J. Johnson, J. Muschelli, G.A. Devenyi, J.T. Duda, S.R. Das, N.C. Cullen, D.L. Gillen, M.A. Yassa, J.R. Stone, J.C. Gee, and B.B. Avants. The ANTsX ecosystem for quantitative biological and medical imaging. Scientific Reports, 11(1):9068 (2021), pp. 1-13.

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Lucius S. Fekonja (lucius.fekonja[at]charite.de) and Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2025 the MRtrix3 contributors.

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

.. _5ttgen_freesurfer:

5ttgen freesurfer
=================

Synopsis
--------

Generate the 5TT image based on a FreeSurfer parcellation image

Usage
-----

::

    5ttgen freesurfer input output [ options ]

-  *input*: The input FreeSurfer parcellation image (any image containing "aseg" in its name)
-  *output*: The output 5TT image

Options
-------

Options specific to the "freesurfer" algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-lut file** Manually provide path to the lookup table on which the input parcellation image is based (e.g. FreeSurferColorLUT.txt)

Options common to all 5ttgen algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocrop** Do NOT crop the resulting 5TT image to reduce its size (keep the same dimensions as the input image)

- **-sgm_amyg_hipp** Represent the amygdalae and hippocampi as sub-cortical grey matter in the 5TT image

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify an existing directory in which to generate the scratch directory.

- **-continue ScratchDir LastFile** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

Standard options
^^^^^^^^^^^^^^^^

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages.

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

- **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

* Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. NeuroImage, 2012, 62, 1924-1938

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2025 the MRtrix3 contributors.

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

.. _5ttgen_fsl:

5ttgen fsl
==========

Synopsis
--------

Use FSL commands to generate the 5TT image based on a T1-weighted image

Usage
-----

::

    5ttgen fsl input output [ options ]

-  *input*: The input T1-weighted image
-  *output*: The output 5TT image

Options
-------

Options specific to the "fsl" algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-t2 image** Provide a T2-weighted image in addition to the default T1-weighted image; this will be used as a second input to FSL FAST

- **-mask image** Manually provide a brain mask, rather than deriving one in the script

- **-premasked** Indicate that brain masking has already been applied to the input image

Options common to all 5ttgen algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocrop** Do NOT crop the resulting 5TT image to reduce its size (keep the same dimensions as the input image)

- **-sgm_amyg_hipp** Represent the amygdalae and hippocampi as sub-cortical grey matter in the 5TT image

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify an existing directory in which to generate the scratch directory.

- **-continue ScratchDir LastFile** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

Standard options
^^^^^^^^^^^^^^^^

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages.

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

- **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

* Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. NeuroImage, 2012, 62, 1924-1938

* Smith, S. M. Fast robust automated brain extraction. Human Brain Mapping, 2002, 17, 143-155

* Zhang, Y.; Brady, M. & Smith, S. Segmentation of brain MR images through a hidden Markov random field model and the expectation-maximization algorithm. IEEE Transactions on Medical Imaging, 2001, 20, 45-57

* Patenaude, B.; Smith, S. M.; Kennedy, D. N. & Jenkinson, M. A Bayesian model of shape and appearance for subcortical brain segmentation. NeuroImage, 2011, 56, 907-922

* Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2025 the MRtrix3 contributors.

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

.. _5ttgen_gif:

5ttgen gif
==========

Synopsis
--------

Generate the 5TT image based on a Geodesic Information Flow (GIF) segmentation image

Usage
-----

::

    5ttgen gif input output [ options ]

-  *input*: The input Geodesic Information Flow (GIF) segmentation image
-  *output*: The output 5TT image

Options
-------

Options common to all 5ttgen algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocrop** Do NOT crop the resulting 5TT image to reduce its size (keep the same dimensions as the input image)

- **-sgm_amyg_hipp** Represent the amygdalae and hippocampi as sub-cortical grey matter in the 5TT image

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify an existing directory in which to generate the scratch directory.

- **-continue ScratchDir LastFile** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

Standard options
^^^^^^^^^^^^^^^^

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages.

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

- **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

* Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. NeuroImage, 2012, 62, 1924-1938

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Matteo Mancini (m.mancini@ucl.ac.uk)

**Copyright:** Copyright (c) 2008-2025 the MRtrix3 contributors.

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

.. _5ttgen_hsvs:

5ttgen hsvs
===========

Synopsis
--------

Generate a 5TT image based on Hybrid Surface and Volume Segmentation (HSVS), using FreeSurfer and FSL tools

Usage
-----

::

    5ttgen hsvs input output [ options ]

-  *input*: The input FreeSurfer subject directory
-  *output*: The output 5TT image

Options
-------

- **-freesurfer_lut file** Manually provide the path to the FreeSurfer lookup table file

- **-template image** Provide an image that will form the template for the generated 5TT image

- **-hippocampi choice** Select method to be used for hippocampi (& amygdalae) segmentation; options are: subfields,first,aseg

- **-thalami choice** Select method to be used for thalamic segmentation; options are: nuclei,first,aseg

- **-white_stem** Classify the brainstem as white matter; streamlines will not be permitted to terminate within this region

Options common to all 5ttgen algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocrop** Do NOT crop the resulting 5TT image to reduce its size (keep the same dimensions as the input image)

- **-sgm_amyg_hipp** Represent the amygdalae and hippocampi as sub-cortical grey matter in the 5TT image

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify an existing directory in which to generate the scratch directory.

- **-continue ScratchDir LastFile** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

Standard options
^^^^^^^^^^^^^^^^

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages.

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

- **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

- **-help** display this information page and exit.

- **-version** display version information and exit.

References
^^^^^^^^^^

* Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. NeuroImage, 2012, 62, 1924-1938

* Smith, R.; Skoch, A.; Bajada, C.; Caspers, S.; Connelly, A. Hybrid Surface-Volume Segmentation for improved Anatomically-Constrained Tractography. In Proc OHBM 2020

* Fischl, B. Freesurfer. NeuroImage, 2012, 62(2), 774-781

* If FreeSurfer hippocampal subfields module is utilised: Iglesias, J.E.; Augustinack, J.C.; Nguyen, K.; Player, C.M.; Player, A.; Wright, M.; Roy, N.; Frosch, M.P.; Mc Kee, A.C.; Wald, L.L.; Fischl, B.; and Van Leemput, K. A computational atlas of the hippocampal formation using ex vivo, ultra-high resolution MRI: Application to adaptive segmentation of in vivo MRI. NeuroImage, 2015, 115, 117-137

* If FreeSurfer hippocampal subfields module is utilised and includes amygdalae segmentation: Saygin, Z.M. & Kliemann, D.; Iglesias, J.E.; van der Kouwe, A.J.W.; Boyd, E.; Reuter, M.; Stevens, A.; Van Leemput, K.; Mc Kee, A.; Frosch, M.P.; Fischl, B.; Augustinack, J.C. High-resolution magnetic resonance imaging reveals nuclei of the human amygdala: manual segmentation to automatic atlas. NeuroImage, 2017, 155, 370-382

* If -thalami nuclei is used: Iglesias, J.E.; Insausti, R.; Lerma-Usabiaga, G.; Bocchetta, M.; Van Leemput, K.; Greve, D.N.; van der Kouwe, A.; ADNI; Fischl, B.; Caballero-Gaudes, C.; Paz-Alonso, P.M. A probabilistic atlas of the human thalamic nuclei combining ex vivo MRI and histology. NeuroImage, 2018, 183, 314-326

* If ACPCDetect is installed: Ardekani, B.; Bachman, A.H. Model-based automatic detection of the anterior and posterior commissures on MRI scans. NeuroImage, 2009, 46(3), 677-682

* If FSL FIRST is used for subcortical structures: Patenaude, B.; Smith, S. M.; Kennedy, D. N. & Jenkinson, M. A Bayesian model of shape and appearance for subcortical brain segmentation. NeuroImage, 2011, 56, 907-922

* If FSL is installed: Zhang, Y.; Brady, M. & Smith, S. Segmentation of brain MR images through a hidden Markov random field model and the expectation-maximization algorithm. IEEE Transactions on Medical Imaging, 2001, 20, 45-57

* If FSL is installed: Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2025 the MRtrix3 contributors.

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

