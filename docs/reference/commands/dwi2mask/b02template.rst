.. _dwi2mask_b02template:

dwi2mask b02template
====================

Synopsis
--------

Register the mean b=0 image to a T2-weighted template to back-propagate a brain mask

Usage
-----

::

    dwi2mask b02template input output [ options ]

-  *input*: The input DWI series
-  *output*: The output mask image

Description
-----------

This script currently assumes that the template image provided via the first input to the -template option is T2-weighted, and can therefore be trivially registered to a mean b=0 image.

Command-line option -ants_options can be used with either the "antsquick" or "antsfull" software options. In both cases, image dimensionality is assumed to be 3, and so this should be omitted from the user-specified options.The input can be either a string (encased in double-quotes if more than one option is specified), or a path to a text file containing the requested options. In the case of the "antsfull" software option, one will require the names of the fixed and moving images that are provided to the antsRegistration command: these are "template_image.nii" and "bzero.nii" respectively.

Options
-------

Options for importing the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion gradient table in MRtrix format

-  **-fslgrad bvecs bvals** Provide the diffusion gradient table in FSL bvecs/bvals format

*(these options are mutually exclusive; at most one may be specified)*


Options specific to the "template" algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-software choice** The software to use for template registration (default: antsquick, unless overridden by the Dwi2maskTemplateSoftware config file entry) (choices: antsfull, antsquick, fsl)

-  **-template TemplateImage MaskImage** Provide the template image to which the input data will be registered, and the mask to be projected to the input image. The template image should be T2-weighted.

Options applicable when using the ANTs software for registration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-ants_options " ANTsOptions"** Provide options to be passed to the ANTs registration command (see Description)

Options applicable when using the FSL software for registration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-flirt_options " FlirtOptions"** Command-line options to pass to the FSL flirt command (provide a string within quotation marks that contains at least one space, even if only passing a single command-line option to flirt)

-  **-fnirt_config file** Specify a FNIRT configuration file for registration

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

If FSL software is used for registration: M. Jenkinson, C.F. Beckmann, T.E. Behrens, M.W. Woolrich, S.M. Smith. FSL. NeuroImage, 62:782-90, 2012

If ANTs software is used for registration: B. Avants, N.J. Tustison, G. Song, P.A. Cook, A. Klein, J.C. Jee. A reproducible evaluation of ANTs similarity metric performance in brain image registration. NeuroImage, 2011, 54, 2033-2044

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

