mesh2pve
===========

Synopsis
--------

::

    mesh2pve [ options ]  source template output

-  *source*: the mesh file; note vertices must be defined in realspace
   coordinates
-  *template*: the template image
-  *output*: the output image

Description
-----------

convert a mesh surface to a partial volume estimation image.

Options
-------

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
Anatomically-constrained tractography: Improved diffusion MRI
streamlines tractography through effective use of anatomical
information. NeuroImage, 2012, 62, 1924-1938

--------------

MRtrix new_syntax-1436-ge228c30b, built Feb 17 2016

**Author:** Robert E. Smith (r.smith@brain.org.au)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
