5tt2gmwmi
===========

Synopsis
--------

::

    5tt2gmwmi [ options ]  5tt_in mask_out

-  *5tt_in*: the input 5TT segmented anatomical image
-  *mask_out*: the output mask image

Description
-----------

Generate a mask image appropriate for seeding streamlines on the grey
matter - white matter interface

Options
-------

-  **-mask_in image** Filter an input mask image according to those
   voxels that lie upon the grey matter - white matter boundary. If no
   input mask is provided, the output will be a whole-brain mask image
   calculated using the anatomical image only.

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

References
^^^^^^^^^^

Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A.
Anatomically-constrained tractography:Improved diffusion MRI streamlines
tractography through effective use of anatomical information.
NeuroImage, 2012, 62, 1924-1938

--------------

MRtrix new_syntax-1436-ge228c30b, built Feb 17 2016

**Author:** Robert E. Smith (r.smith@brain.org.au)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
