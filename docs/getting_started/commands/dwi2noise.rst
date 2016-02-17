dwi2noise
===========

Synopsis
--------

::

    dwi2noise [ options ]  dwi noise

-  *dwi*: the input diffusion-weighted image.
-  *noise*: the output noise map

Description
-----------

estimate noise level voxel-wise using residuals from a truncated SH fit

Options
-------

-  **-lmax order** set the maximum harmonic order for the output
   series. By default, the program will use the highest possible lmax
   given the number of diffusion-weighted images up to lmax = 8.

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad encoding** specify the diffusion-weighted gradient scheme
   used in the acquisition. The program will normally attempt to use the
   encoding stored in the image header. This should be supplied as a 4xN
   text file with each line is in the format [ X Y Z b ], where [ X Y Z
   ] describe the direction of the applied gradient, and b gives the
   b-value in units of s/mm^2.

-  **-fslgrad bvecs bvals** specify the diffusion-weighted gradient
   scheme used in the acquisition in FSL bvecs/bvals format.

-  **-bvalue_scaling mode** specifies whether the b-values should be
   scaled by the square of the corresponding DW gradient norm, as often
   required for multi-shell or DSI DW acquisition schemes. The default
   action can also be set in the MRtrix config file, under the
   BValueScaling entry. Valid choices are yes/no, true/false, 0/1
   (default: true).

DW Shell selection options
^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-shell list** specify one or more diffusion-weighted gradient
   shells to use during processing, as a comma-separated list of the
   desired approximate b-values. Note that some commands are
   incompatible with multiple shells, and will throw an error if more
   than one b-value are provided.

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files. Caution: Using the same
   file as input and output might cause unexpected behaviour.

-  **-nthreads number** use this number of threads in multi-threaded
   applications

-  **-failonwarn** terminate program if a warning is produced

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

--------------

MRtrix new_syntax-1436-ge228c30b, built Feb 17 2016

**Author:** J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
