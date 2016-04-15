.. _msdwi2fod:

msdwi2fod
===========

Synopsis
--------

::

    msdwi2fod [ options ]  dwi response odf [ response odf ... ]

-  *dwi*: the input dwi image.
-  *response odf*: the input tissue response and the output ODF image.

Description
-----------

Multi-shell, multi-tissue constrained spherical deconvolution.

Options
-------

-  **-mask image** only perform computation within the specified binary brain mask image.

-  **-lmax order** the lmax values to use per tissue type, as a comma-separated list of even integers.

-  **-directions file** specify the directions over which to apply the non-negativity constraint (by default, the built-in 300 direction set is used). These should be supplied as a text file containing the [ az el ] pairs for the directions.

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad encoding** specify the diffusion-weighted gradient scheme used in the acquisition. The program will normally attempt to use the encoding stored in the image header. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2.

-  **-fslgrad bvecs bvals** specify the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format.

-  **-bvalue_scaling mode** specifies whether the b-values should be scaled by the square of the corresponding DW gradient norm, as often required for multi-shell or DSI DW acquisition schemes. The default action can also be set in the MRtrix config file, under the BValueScaling entry. Valid choices are yes/no, true/false, 0/1 (default: true).

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

Jeurissen, B; Tournier, J-D; Dhollander, T; Connelly, A & Sijbers, J. Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data NeuroImage, 2014, 103, 411-426

--------------



**Author:** Ben Jeurissen (ben.jeurissen@uantwerpen.be)

**Copyright:** Copyright (C) 2015 Vision Lab, University of Antwerp, Belgium. This is free software; see the source for copying conditions. There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

