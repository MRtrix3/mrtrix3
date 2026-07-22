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

-  **-freesurfer_lut file** Manually provide the path to the FreeSurfer lookup table file

-  **-template image** Provide an image that will form the template for the generated 5TT image

-  **-hippocampi choice** Select method to be used for hippocampi (& amygdalae) segmentation (choices: subfields, first, aseg)

-  **-thalami choice** Select method to be used for thalamic segmentation (choices: nuclei, first, aseg)

-  **-white_stem** Classify the brainstem as white matter; streamlines will not be permitted to terminate within this region

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

Smith, R.; Skoch, A.; Bajada, C.; Caspers, S.; Connelly, A. Hybrid Surface-Volume Segmentation for improved Anatomically-Constrained Tractography. In Proc OHBM 2020

Fischl, B. Freesurfer. NeuroImage, 2012, 62(2), 774-781

If FreeSurfer hippocampal subfields module is utilised: Iglesias, J.E.; Augustinack, J.C.; Nguyen, K.; Player, C.M.; Player, A.; Wright, M.; Roy, N.; Frosch, M.P.; Mc Kee, A.C.; Wald, L.L.; Fischl, B.; and Van Leemput, K. A computational atlas of the hippocampal formation using ex vivo, ultra-high resolution MRI: Application to adaptive segmentation of in vivo MRI. NeuroImage, 2015, 115, 117-137

If FreeSurfer hippocampal subfields module is utilised and includes amygdalae segmentation: Saygin, Z.M. & Kliemann, D.; Iglesias, J.E.; van der Kouwe, A.J.W.; Boyd, E.; Reuter, M.; Stevens, A.; Van Leemput, K.; Mc Kee, A.; Frosch, M.P.; Fischl, B.; Augustinack, J.C. High-resolution magnetic resonance imaging reveals nuclei of the human amygdala: manual segmentation to automatic atlas. NeuroImage, 2017, 155, 370-382

If -thalami nuclei is used: Iglesias, J.E.; Insausti, R.; Lerma-Usabiaga, G.; Bocchetta, M.; Van Leemput, K.; Greve, D.N.; van der Kouwe, A.; ADNI; Fischl, B.; Caballero-Gaudes, C.; Paz-Alonso, P.M. A probabilistic atlas of the human thalamic nuclei combining ex vivo MRI and histology. NeuroImage, 2018, 183, 314-326

If ACPCDetect is installed: Ardekani, B.; Bachman, A.H. Model-based automatic detection of the anterior and posterior commissures on MRI scans. NeuroImage, 2009, 46(3), 677-682

If FSL FIRST is used for subcortical structures: Patenaude, B.; Smith, S. M.; Kennedy, D. N. & Jenkinson, M. A Bayesian model of shape and appearance for subcortical brain segmentation. NeuroImage, 2011, 56, 907-922

If FSL is installed: Zhang, Y.; Brady, M. & Smith, S. Segmentation of brain MR images through a hidden Markov random field model and the expectation-maximization algorithm. IEEE Transactions on Medical Imaging, 2001, 20, 45-57

If FSL is installed: Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219

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

