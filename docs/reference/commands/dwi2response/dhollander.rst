.. _dwi2response_dhollander:

dwi2response dhollander
=======================

Synopsis
--------

Unsupervised estimation of WM, GM and CSF response functions that does not require a T1 image (or segmentation thereof)

Usage
-----

::

    dwi2response dhollander input out_sfwm out_gm out_csf [ options ]

-  *input*: Input DWI dataset
-  *out_sfwm*: Output single-fibre WM response function text file
-  *out_gm*: Output GM response function text file
-  *out_csf*: Output CSF response function text file

Description
-----------

This is an improved version of the Dhollander et al. (2016) algorithm for unsupervised estimation of WM, GM and CSF response functions, which includes the Dhollander et al. (2019) improvements for single-fibre WM response function estimation (prior to this update, the "dwi2response tournier" algorithm had been utilised specifically for the single-fibre WM response function estimation step).

Options
-------

General dwi2response options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-mask image** Provide an initial mask for response voxel selection

- **-voxels image** Output an image showing the final voxel selection(s)

- **-shells bvalues** The b-value(s) to use in response function estimation (comma-separated list in case of multiple b-values; b=0 must be included explicitly if desired)

- **-lmax values** The maximum harmonic degree(s) for response function estimation (comma-separated list in case of multiple b-values) (values must be non-negative and even)

Options for importing the diffusion gradient table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-grad file** Provide the diffusion gradient table in MRtrix format

- **-fslgrad bvecs bvals** Provide the diffusion gradient table in FSL bvecs/bvals format

Options for the "dhollander" algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-erode iterations** Number of erosion passes to apply to initial (whole brain) mask. Set to 0 to not erode the brain mask. (minimum: 0) (default: 3)

- **-fa threshold** FA threshold for crude WM versus GM-CSF separation. (range: 0 to 1) (default: 0.2)

- **-sfwm percentage** Final number of single-fibre WM voxels to select, as a percentage of refined WM. (range: 0 to 100) (default: 0.5 per cent)

- **-gm percentage** Final number of GM voxels to select, as a percentage of refined GM. (range: 0 to 100) (default: 2 per cent)

- **-csf percentage** Final number of CSF voxels to select, as a percentage of refined CSF. (range: 0 to 100) (default: 10 per cent)

- **-wm_algo algorithm** Use external dwi2response algorithm for WM single-fibre voxel selection (choices: fa, tax, tournier) (default: built-in Dhollander 2019)

Standard options
^^^^^^^^^^^^^^^^

- **-force** force overwrite of output files.

- **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading). (minimum: 0)

- **-config key value**  *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

- **-help** display this information page and exit.

- **-version** display version information and exit.

Verbosity options
"""""""""""""""""

- **-info** display information messages.

- **-quiet** do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

- **-debug** display debugging messages & debug input data.

Additional standard options for Python scripts
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-nocleanup** do not delete intermediate files during script execution, and do not delete scratch directory at script completion.

- **-scratch /path/to/scratch/** manually specify an existing directory in which to generate the scratch directory.

- **-continue ScratchDir LastFile** continue the script from a previous execution; must provide the scratch directory path, and the name of the last successfully-generated file.

References
^^^^^^^^^^

* Dhollander, T.; Raffelt, D. & Connelly, A. Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image. ISMRM Workshop on Breaking the Barriers of Diffusion MRI, 2016, 5

* If -wm_algo option is not used: Dhollander, T.; Mito, R.; Raffelt, D. & Connelly, A. Improved white matter response function estimation for 3-tissue constrained spherical deconvolution. Proc Intl Soc Mag Reson Med, 2019, 555

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Thijs Dhollander (thijs.dhollander@gmail.com)

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

