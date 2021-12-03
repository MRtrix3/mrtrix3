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

-  *algorithm*: Select the algorithm to be used to complete the script operation; additional details and options become available once an algorithm is nominated. Options are: dhcp, freesurfer, fsl, gif, hsvs, mcrib

Description
-----------

5ttgen acts as a 'master' script for generating a five-tissue-type (5TT) segmented tissue image suitable for use in Anatomically-Constrained Tractography (ACT). A range of different algorithms are available for completing this task. When using this script, the name of the algorithm to be used must appear as the first argument on the command-line after '5ttgen'. The subsequent compulsory arguments and options available depend on the particular algorithm being invoked.

Each algorithm available also has its own help page, including necessary references; e.g. to see the help page of the 'fsl' algorithm, type '5ttgen fsl'.

Options
-------

Options common to all 5ttgen algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocrop** Do NOT crop the resulting 5TT image to reduce its size (keep the same dimensions as the input image)

- **-sgm_amyg_hipp** Represent the amygdalae and hippocampi as sub-cortical grey matter in the 5TT image

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify the path in which to generate the scratch directory.

- **-continue <ScratchDir> <LastFile>** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

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

**Copyright:** Copyright (c) 2008-2021 the MRtrix3 contributors.

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

.. _5ttgen_dhcp:

5ttgen dhcp
===========

Synopsis
--------

Use ANTs commands, the output of the dHCP pipeline and the M-CRIB atlas to generate the 5TT image of a neonatal subject based on a T2-weighted image

Usage
-----

::

    5ttgen dhcp input output [ options ]

-  *input*: The input path of the derivates folder (anat/) obtained from the dHCP pipeline
-  *output*: The output 5TT image

Description
-----------

Derivation of the 5TT image is principally based on the segmentation already performed in the dHCP pipeline. The M-CRIB atlas will only be used to introduce additional sub-cortical grey matter parcels into the tissue segmentation. By default, the algorithm will use the tissue probability maps obtained from the dHCP. However, if the pipeline was executed without the -additional command-line flag, the algorithm will use the hard segmentation.

Options
-------

Options specific to the 'dhcp' algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-mcrib_path** Provide the path of the M-CRIB atlas (note: this can alternatively be specified in the MRtrix config file as "MCRIBPath")

- **-parcellation** Additionally export the M-CRIB parcellation warped to the subject data

- **-quick** Specify the use of quick registration parameters

- **-hard_segmentation** Specify the use of hard segmentation instead of the soft segmentation to generate the 5TT

- **-ants_parallel** Control for parallel computation for antsJointLabelFusion (default 0) -- 0 == run serially,  1 == SGE qsub, 2 == use PEXEC (localhost), 3 == Apple XGrid, 4 == PBS qsub, 5 == SLURM.

Options common to all 5ttgen algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocrop** Do NOT crop the resulting 5TT image to reduce its size (keep the same dimensions as the input image)

- **-sgm_amyg_hipp** Represent the amygdalae and hippocampi as sub-cortical grey matter in the 5TT image

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify the path in which to generate the scratch directory.

- **-continue <ScratchDir> <LastFile>** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

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

* Blesa, M.; Galdi, P.; Cox, S.R.; Sullivan, G.; Stoye, D.Q.; Lamb, G.L.; Quigley, A.J.; Thrippleton, M.J.; Escudero, J.; Bastin, M.E.; Smith, K.M. & Boardman. J.P. Hierarchical complexity of the macro-scale neonatal brain. Cerebral Cortex, 2021, 4, 2071-2084

* Avants, B.; Epstein, C.; Grossman, M. & Gee, J. Symmetric diffeomorphic image registration with cross-correlation: evaluating automated labeling of elderly and neurodegenerative brain. 2008, Medical Image Analysis, 41, 12-26

* Wang, H.; Suh, J.W.; Das, S.R.; Pluta, J.B.; Craige, C. & Yushkevich, P.A. Multi-atlas segmentation with joint label fusion. IEEE Trans Pattern Anal Mach Intell., 2013, 35, 611-623.

* Alexander, B.; Murray, A.L.; Loh, W.Y.; Matthews, L.G.; Adamson, C.; Beare, R.; Chen, J.; Kelly, C.E.; Rees, S.; Warfield, S.K.; Anderson, P.J.; Doyle, L.W.; Spittle, A.J.; Cheong, J.L.Y; Seal, M.L. & Thompson, D.K. A new neonatal cortical and subcortical brain atlas: the Melbourne Children's Regional Infant Brain (m-crib) atlas. NeuroImage, 2017, 852, 147-841.

* Makropoulos, A., Robinson, E.C., Schuh, A., Wright, R., Fitzgibbon, S., Bozek, J., Counsell, S.J., Steinweg, J., Vecchiato, K., Passerat-Palmbach, J., Lenz, G., Mortari, F., Tenev, T., Duff, E.P., Bastiani, M., Cordero-Grande, L., Hughes, E., Tusor, N., Tournier, J.D., Hutter, J., Price, A.N., Teixeira, R.P.A.G., Murgasova, M., Victor, S., Kelly, C., Rutherford, M.A., Smith, S.M., Edwards, A.D., Hajnal, J.V., Jenkinson, M. & Rueckert, D. The developing human connectome project: A minimal processing pipeline for neonatal cortical surface reconstruction. NeuroImage, 2018, 173, 88-112.

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Manuel Blesa (manuel.blesa@ed.ac.uk), Paola Galdi (paola.galdi@ed.ac.uk) and Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2021 the MRtrix3 contributors.

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

-  *input*: The input FreeSurfer parcellation image (any image containing 'aseg' in its name)
-  *output*: The output 5TT image

Options
-------

Options specific to the 'freesurfer' algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-lut** Manually provide path to the lookup table on which the input parcellation image is based (e.g. FreeSurferColorLUT.txt)

Options common to all 5ttgen algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocrop** Do NOT crop the resulting 5TT image to reduce its size (keep the same dimensions as the input image)

- **-sgm_amyg_hipp** Represent the amygdalae and hippocampi as sub-cortical grey matter in the 5TT image

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify the path in which to generate the scratch directory.

- **-continue <ScratchDir> <LastFile>** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

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

**Copyright:** Copyright (c) 2008-2021 the MRtrix3 contributors.

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

Options specific to the 'fsl' algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-t2 <T2 image>** Provide a T2-weighted image in addition to the default T1-weighted image; this will be used as a second input to FSL FAST

- **-mask** Manually provide a brain mask, rather than deriving one in the script

- **-premasked** Indicate that brain masking has already been applied to the input image

Options common to all 5ttgen algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocrop** Do NOT crop the resulting 5TT image to reduce its size (keep the same dimensions as the input image)

- **-sgm_amyg_hipp** Represent the amygdalae and hippocampi as sub-cortical grey matter in the 5TT image

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify the path in which to generate the scratch directory.

- **-continue <ScratchDir> <LastFile>** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

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

**Copyright:** Copyright (c) 2008-2021 the MRtrix3 contributors.

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

- **-scratch /path/to/scratch/** manually specify the path in which to generate the scratch directory.

- **-continue <ScratchDir> <LastFile>** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

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

**Copyright:** Copyright (c) 2008-2021 the MRtrix3 contributors.

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

- **-template** Provide an image that will form the template for the generated 5TT image

- **-hippocampi** Select method to be used for hippocampi (& amygdalae) segmentation; options are: subfields,first,aseg

- **-thalami** Select method to be used for thalamic segmentation; options are: nuclei,first,aseg

- **-white_stem** Classify the brainstem as white matter

Options common to all 5ttgen algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocrop** Do NOT crop the resulting 5TT image to reduce its size (keep the same dimensions as the input image)

- **-sgm_amyg_hipp** Represent the amygdalae and hippocampi as sub-cortical grey matter in the 5TT image

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify the path in which to generate the scratch directory.

- **-continue <ScratchDir> <LastFile>** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

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

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2021 the MRtrix3 contributors.

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

.. _5ttgen_mcrib:

5ttgen mcrib
============

Synopsis
--------

Use ANTs commands and the M-CRIB atlas to generate the 5TT image of a neonatal subject based on a T1-weighted or T2-weighted image

Usage
-----

::

    5ttgen mcrib input modality output [ options ]

-  *input*: The input structural image
-  *modality*: Specify the modality of the input image, either "t1w" or "t2w"
-  *output*: The output 5TT image

Description
-----------

This command creates the 5TT file for human neonatal subjects. The M-CRIB atlas is used to idenity the different tissues.

Options
-------

Options specific to the 'mcrib' algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-mask** Manually provide a brain mask, MANDATORY

- **-mcrib_path** Provide the path of the M-CRIB atlas (note: this can alternatively be specified in the MRtrix config file as "MCRIBPath")

- **-parcellation** Additionally export the M-CRIB parcellation warped to the subject data

- **-quick** Specify the use of quick registration parameters

- **-hard_segmentation** Specify the use of hard segmentation instead of the soft segmentation to generate the 5TT (NOTE: use of this option in this segmentation algorithm is not recommended)

- **-ants_parallel** Control for parallel computation for antsJointLabelFusion (default 0) -- 0 == run serially,  1 == SGE qsub, 2 == use PEXEC (localhost), 3 == Apple XGrid, 4 == PBS qsub, 5 == SLURM.

Options common to all 5ttgen algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocrop** Do NOT crop the resulting 5TT image to reduce its size (keep the same dimensions as the input image)

- **-sgm_amyg_hipp** Represent the amygdalae and hippocampi as sub-cortical grey matter in the 5TT image

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify the path in which to generate the scratch directory.

- **-continue <ScratchDir> <LastFile>** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

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

* Blesa, M.; Galdi, P.; Cox, S.R.; Sullivan, G.; Stoye, D.Q.; Lamb, G.L.; Quigley, A.J.; Thrippleton, M.J.; Escudero, J.; Bastin, M.E.; Smith, K.M. & Boardman. J.P. Hierarchical complexity of the macro-scale neonatal brain. Cerebral Cortex, 2021, 4, 2071-2084

* Avants, B.; Epstein, C.; Grossman, M. & Gee, J. Symmetric diffeomorphic image registration with cross-correlation: evaluating automated labeling of elderly and neurodegenerative brain. 2008, Medical Image Analysis, 41, 12-26

* Wang, H.; Suh, J.W.; Das, S.R.; Pluta, J.B.; Craige, C. & Yushkevich, P.A. Multi-atlas segmentation with joint label fusion. IEEE Trans Pattern Anal Mach Intell., 2013, 35, 611-623.

* Alexander, B.; Murray, A.L.; Loh, W.Y.; Matthews, L.G.; Adamson, C.; Beare, R.; Chen, J.; Kelly, C.E.; Rees, S.; Warfield, S.K.; Anderson, P.J.; Doyle, L.W.; Spittle, A.J.; Cheong, J.L.Y; Seal, M.L. & Thompson, D.K. A new neonatal cortical and subcortical brain atlas: the Melbourne Children's Regional Infant Brain (m-crib) atlas. NeuroImage, 2017, 852, 147-841.

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Manuel Blesa (manuel.blesa@ed.ac.uk), Paola Galdi (paola.galdi@ed.ac.uk) and Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2021 the MRtrix3 contributors.

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

