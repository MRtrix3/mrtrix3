Scripts for external libraries
======

This section provides the documentation for those scripts provided with
*MRtrix3* that have been built around the Python libraries (it therefore
does not list those scripts that are written in Bash). This is the same
information that is displayed by each script when invoked without
options, or with the ``-help`` option.

``revpe_dwicombine``
--------

Synopsis
^^^^^^^^

::

    revpe_dwicombine [ options ] pe_axis series1 series2 output

-  *pe\_axis*: The phase encode direction / axis; can be an axis number
   (0, 1 or 2) or a code (e.g. AP, LR, IS)
-  *series1*: The first input image series
-  *series2*: The second input image series
-  *output*: The output image series

Description
^^^^^^^^^^^

Perform EPI distortion & recombination of a pair of image volumes, where
the diffusion gradient table is identical between the two series, but
the phase-encode direction is reversed

Standard options
""""""""""""""""

-  **[ -h --help ]**\ show this help message and exit

-  **-continue **\ Continue the script from a previous execution; must
   provide the temporary directory path, and the name of the last
   successfully-generated file

-  **-nocleanup**\ Do not delete temporary directory at script
   completion

-  **-nthreads number**\ Use this number of threads in MRtrix
   multi-threaded applications (0 disables multi-threading)

-  **-tempdir /path/to/tmp/**\ Manually specify the path in which to
   generate the temporary directory

-  **-quiet**\ Suppress all console output during script execution

-  **-verbose**\ Display additional information for every command
   invoked

References
""""""""""

eddy/topup: Andersson, J. L. & Sotiropoulos, S. N. An integrated
approach to correction for off-resonance effects and subject movement in
diffusion MR imaging. NeuroImage, doi:10.1016/j.neuroimage.2015.10.019

FSL: Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.;
Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.;
Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.;
Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in
functional and structural MR image analysis and implementation as FSL.
NeuroImage, 2004, 23, S208-S219

Skare2010: Skare, S. & Bammer, R. Jacobian weighting of distortion
corrected EPI data. Proceedings of the International Society for
Magnetic Resonance in Medicine, 2010, 5063

--------------

**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.


``revpe_distcorr``
---------

Synopsis
^^^^^^^

::

    revpe_distcorr [ options ] pe_axis image1of2 image2of2 series output

-  *pe\_axis*: The phase encode direction / axis; can be an axis number
   (0, 1 or 2) or a code (e.g. AP, LR, IS)
-  *image1of2*: The first image of the reversed-PE image pair
-  *image2of2*: The second image of the reversed-PE image pair
-  *series*: The image series to be corrected; note that the
   phase-encode direction of this series should be identical to the
   FIRST image of the reversed-PE pair
-  *output*: The output corrected image series

Description
^^^^^^^^^^

Perform EPI distortion correction of a volume series using a reversed
phase-encode image pair to estimate the inhomogeneity field

Standard options
"""""""""""""""

-  **[ -h --help ]**\ show this help message and exit

-  **-continue **\ Continue the script from a previous execution; must
   provide the temporary directory path, and the name of the last
   successfully-generated file

-  **-nocleanup**\ Do not delete temporary directory at script
   completion

-  **-nthreads number**\ Use this number of threads in MRtrix
   multi-threaded applications (0 disables multi-threading)

-  **-tempdir /path/to/tmp/**\ Manually specify the path in which to
   generate the temporary directory

-  **-quiet**\ Suppress all console output during script execution

-  **-verbose**\ Display additional information for every command
   invoked

References
""""""""""

eddy/topup: Andersson, J. L. & Sotiropoulos, S. N. An integrated
approach to correction for off-resonance effects and subject movement in
diffusion MR imaging. NeuroImage, doi:10.1016/j.neuroimage.2015.10.019

FSL: Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.;
Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.;
Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.;
Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in
functional and structural MR image analysis and implementation as FSL.
NeuroImage, 2004, 23, S208-S219

--------------

**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.


``fs_parc_replace_sgm_first``
--------

Synopsis
^^^^^^^^

::

    fs_parc_replace_sgm_first [ options ] parc t1 config output

-  *parc*: The input FreeSurfer parcellation image
-  *t1*: The T1 image to be provided to FIRST
-  *config*: Either the FreeSurfer lookup table (if the input
   parcellation image is directly from FreeSurfer), or the connectome
   configuration file (if the parcellation image has been modified using
   the labelconfig command)
-  *output*: The output parcellation image

Description
^^^^^^^^^^^

In a FreeSurfer parcellation image, replace the sub-cortical grey matter
structure delineations using FSL FIRST

Standard options
""""""""""""""""

-  **[ -h --help ]**\ show this help message and exit

-  **-continue **\ Continue the script from a previous execution; must
   provide the temporary directory path, and the name of the last
   successfully-generated file

-  **-nocleanup**\ Do not delete temporary directory at script
   completion

-  **-nthreads number**\ Use this number of threads in MRtrix
   multi-threaded applications (0 disables multi-threading)

-  **-tempdir /path/to/tmp/**\ Manually specify the path in which to
   generate the temporary directory

-  **-quiet**\ Suppress all console output during script execution

-  **-verbose**\ Display additional information for every command
   invoked

References
""""""""""

first: Patenaude, B.; Smith, S. M.; Kennedy, D. N. & Jenkinson, M. A
Bayesian model of shape and appearance for subcortical brain
segmentation. NeuroImage, 2011, 56, 907-922

FSL: Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.;
Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.;
Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.;
Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in
functional and structural MR image analysis and implementation as FSL.
NeuroImage, 2004, 23, S208-S219

SIFT\_followup: Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly,
A. The effects of SIFT on the reproducibility and biological accuracy of
the structural connectome. NeuroImage, 2015, 104, 253-265

--------------

**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.


``dwibiascorrect``
--------

Synopsis
^^^^^^^^

::

    dwibiascorrect [ options ] input output

-  *input*: The input image series to be corrected
-  *output*: The output corrected image series

Description
^^^^^^^^^^^

Perform B1 field inhomogeneity correction for a DWI volume series

Standard options
""""""""""""""""

-  **[ -h --help ]**\ show this help message and exit

-  **-continue **\ Continue the script from a previous execution; must
   provide the temporary directory path, and the name of the last
   successfully-generated file

-  **-nocleanup**\ Do not delete temporary directory at script
   completion

-  **-nthreads number**\ Use this number of threads in MRtrix
   multi-threaded applications (0 disables multi-threading)

-  **-tempdir /path/to/tmp/**\ Manually specify the path in which to
   generate the temporary directory

-  **-quiet**\ Suppress all console output during script execution

-  **-verbose**\ Display additional information for every command
   invoked

References
""""""""""

fast: Zhang, Y.; Brady, M. & Smith, S. Segmentation of brain MR images
through a hidden Markov random field model and the
expectation-maximization algorithm. IEEE Transactions on Medical
Imaging, 2001, 20, 45-57

FSL: Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.;
Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.;
Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.;
Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in
functional and structural MR image analysis and implementation as FSL.
NeuroImage, 2004, 23, S208-S219

--------------

**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.

``act_anat_prepare_fsl``
--------

Synopsis
^^^^^^^^

::

    act_anat_prepare_fsl [ options ] input output

-  *input*: The input T1 image
-  *output*: The output 5TT image

Description
^^^^^^^^^^

Generate a 5TT image suitable for ACT from a T1 image using FSL tools

Standard options
""""""""""""""""

-  **[ -h --help ]**\ show this help message and exit

-  **-continue **\ Continue the script from a previous execution; must
   provide the temporary directory path, and the name of the last
   successfully-generated file

-  **-nocleanup**\ Do not delete temporary directory at script
   completion

-  **-nthreads number**\ Use this number of threads in MRtrix
   multi-threaded applications (0 disables multi-threading)

-  **-tempdir /path/to/tmp/**\ Manually specify the path in which to
   generate the temporary directory

-  **-quiet**\ Suppress all console output during script execution

-  **-verbose**\ Display additional information for every command
   invoked

References
""""""""""

ACT: Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A.
Anatomically-constrained tractography: Improved diffusion MRI
streamlines tractography through effective use of anatomical
information. NeuroImage, 2012, 62, 1924-1938

bet: Smith, S. M. Fast robust automated brain extraction. Human Brain
Mapping, 2002, 17, 143-155

fast: Zhang, Y.; Brady, M. & Smith, S. Segmentation of brain MR images
through a hidden Markov random field model and the
expectation-maximization algorithm. IEEE Transactions on Medical
Imaging, 2001, 20, 45-57

first: Patenaude, B.; Smith, S. M.; Kennedy, D. N. & Jenkinson, M. A
Bayesian model of shape and appearance for subcortical brain
segmentation. NeuroImage, 2011, 56, 907-922

FSL: Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.;
Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.;
Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.;
Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in
functional and structural MR image analysis and implementation as FSL.
NeuroImage, 2004, 23, S208-S219

--------------

**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.


``act_anat_prepare_freesurfer``
--------

Synopsis
^^^^^^^^

::

    act_anat_prepare_freesurfer [ options ] input output

-  *input*: The input image series; this should be one of the 'aseg'
   images provided by FreeSurfer
-  *output*: The output 5TT image

Description
^^^^^^^^^^

Generate a 5TT image suitable for ACT from a FreeSurfer segmentation

Standard options
""""""""""""""""

-  **[ -h --help ]**\ show this help message and exit

-  **-continue **\ Continue the script from a previous execution; must
   provide the temporary directory path, and the name of the last
   successfully-generated file

-  **-nocleanup**\ Do not delete temporary directory at script
   completion

-  **-nthreads number**\ Use this number of threads in MRtrix
   multi-threaded applications (0 disables multi-threading)

-  **-tempdir /path/to/tmp/**\ Manually specify the path in which to
   generate the temporary directory

-  **-quiet**\ Suppress all console output during script execution

-  **-verbose**\ Display additional information for every command
   invoked

References
""""""""""

ACT: Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A.
Anatomically-constrained tractography: Improved diffusion MRI
streamlines tractography through effective use of anatomical
information. NeuroImage, 2012, 62, 1924-1938

--------------

**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.


