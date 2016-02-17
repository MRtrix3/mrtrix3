dwinormalise
===========

Synopsis
--------

::

    dwinormalise [ options ]  input mask output

-  *input*: the input DWI image containing volumes that are both
   diffusion weighted and b=0
-  *mask*: the input mask image used to normalise the intensity
-  *output*: the output DWI intensity normalised image

Description
-----------

Intensity normalise the b=0 signal within a supplied white matter mask

Options
-------

-  **-intensity value** normalise the b=0 signal to the specified value
   (Default: 1000)

-  **-percentile value** define the percentile of the mask intensties
   used for normalisation. If this option is not supplied then the
   median value (50th percentile) will be normalised to the desired
   intensity value.

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

**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
