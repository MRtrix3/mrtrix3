.. _dwibiasnormmask:

dwibiasnormmask
===============

Synopsis
--------

Perform a combination of bias field correction, intensity normalisation, and mask derivation, for DWI data

Usage
-----

::

    dwibiasnormmask input output_dwi output_mask [ options ]

-  *input*: The input DWI series to be corrected
-  *output_dwi*: The output corrected DWI series
-  *output_mask*: The output DWI mask

Description
-----------

DWI bias field correction, intensity normalisation and masking are inter-related steps, and errors in each may influence other steps. This script is designed to perform all of these steps in an integrated iterative fashion, with the intention of making all steps more robust.

The operation of the algorithm is as follows. An initial mask is defined, either using the default dwi2mask algorithm or as provided by the user. Based on this mask, a sequence of response function estimation, multi-shell multi-tissue CSD, bias field correction (using the mtnormalise command), and intensity normalisation is performed. The default dwi2mask algorithm is then re-executed on the bias-field-corrected DWI series. This sequence of steps is then repeated based on the revised mask, until either a convergence criterion or some number of maximum iterations is reached.

The MRtrix3 mtnormalise command is used to estimate information relating to bias field and intensity normalisation. However its usage in this context is different to its conventional usage. Firstly, while the corrected ODF images are typically used directly following invocation of this command here the estimated bias field and scaling factors are instead used to apply the relevant corrections to the originating DWI data. Secondly, the global intensity scaling that is calculated and applied is typically based on achieving close to a unity sum of tissue signal fractions throughout the masked region. Here, it is instead the b=0 signal in CSF that forms the reference for this global intensity scaling; this is calculated based on the estimated CSF response function and the tissue-specific intensity scaling (this is calculated internally by mtnormalise as part of its optimisation process, but typically subsequently discarded in favour of a single scaling factor for all tissues)

The ODFs estimated within this optimisation procedure are by default of lower maximal spherical harmonic degree than what would be advised for analysis. This is done for computational efficiency. This behaviour can be modified through the -lmax command-line option.

By default, the optimisation procedure will terminate after only two iterations. This is done because it has been observed for some data / configurations that additional iterations can lead to unstable divergence and erroneous results for bias field estimation and masking. For other configurations, it may be preferable to use a greater number of iterations, and allow the iterative algorithm to converge to a stable solution. This can be controlled via the -max_iters command-line option.

Within the optimisation algorithm, derivation of the mask may potentially be performed differently to a conventional mask derivation that is based on a DWI series (where, in many instances, it is actually only the mean b=0 image that is used). Here, the image corresponding to the sum of tissue signal fractions following spherical deconvolution / bias field correction / intensity normalisation is also available,  and this can potentially be used for mask derivation. Available options are as follows. "dwi2mask": Use the MRtrix3 command dwi2mask on the bias-field-corrected DWI series (ie. do not use the ODF tissue sum image for mask derivation); the algorithm to be invoked can be controlled by the user via the MRtrix config file entry "Dwi2maskAlgorithm". "fslbet": Invoke the FSL command "bet" on the ODF tissue sum image. "hdbet": Invoke the HD-BET command on the ODF tissue sum image. "mrthreshold": Invoke the MRtrix3 command "mrthreshold" on the ODF tissue sum image, where an appropriate threshold value will be determined automatically (and some heuristic cleanup of the resulting mask will be performed). "synthstrip": Invoke the FreeSurfer SynthStrip method on the ODF tissue sum image. "threshold": Apply a fixed partial volume threshold of 0.5 to the ODF tissue sum image (and some heuristic cleanup of the resulting mask will be performed).

Options
-------

Options for importing the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-grad file** Provide the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Provide the diffusion gradient table in FSL bvecs/bvals format

Options relevant to the internal optimisation procedure
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-dice value** Set the Dice coefficient threshold for similarity of masks between sequential iterations that will result in termination due to convergence; default = 0.999

- **-init_mask image** Provide an initial mask for the first iteration of the algorithm (if not provided, the default dwi2mask algorithm will be used)

- **-max_iters count** The maximum number of iterations (see Description); default is 2; set to 0 to proceed until convergence

- **-mask_algo algorithm** The algorithm to use for mask estimation, potentially based on the ODF sum image (see Description); default: threshold

- **-lmax values** The maximum spherical harmonic degree for the estimated FODs (see Description); defaults are "4,0,0" for multi-shell  and "4,0" for single-shell data)

Options that modulate the outputs of the script
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-output_bias image** Export the final estimated bias field to an image

- **-output_scale file** Write the scaling factor applied to the DWI series to a text file

- **-output_tissuesum image** Export the tissue sum image that was used to generate the final mask

- **-reference value** Set the target CSF b=0 intensity in the output DWI series (default: 1000.0)

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify the path in which to generate the scratch directory.

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

* Jeurissen, B; Tournier, J-D; Dhollander, T; Connelly, A & Sijbers, J. Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data. NeuroImage, 2014, 103, 411-426

* Raffelt, D.; Dhollander, T.; Tournier, J.-D.; Tabbara, R.; Smith, R. E.; Pierre, E. & Connelly, A. Bias Field Correction and Intensity Normalisation for Quantitative Analysis of Apparent Fibre Density. In Proc. ISMRM, 2017, 26, 3541

* Dhollander, T.; Raffelt, D. & Connelly, A. Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5

* Dhollander, T.; Tabbara, R.; Rosnarho-Tornstrand, J.; Tournier, J.-D.; Raffelt, D. & Connelly, A. Multi-tissue log-domain intensity and inhomogeneity normalisation for quantitative apparent fibre density. In Proc. ISMRM, 2021, 29, 2472

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and Arshiya Sangchooli (asangchooli@student.unimelb.edu.au)

**Copyright:** Copyright (c) 2008-2024 the MRtrix3 contributors.

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

