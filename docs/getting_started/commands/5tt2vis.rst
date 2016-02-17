5tt2vis
===========

Synopsis
--------

::

    5tt2vis [ options ]  input output

-  *input*: the input 4D tissue-segmented image
-  *output*: the output 3D image for visualisation

Description
-----------

generate an image for visualisation purposes from an ACT 5TT segmented
anatomical image

Options
-------

-  **-bg value** image intensity of background

-  **-cgm value** image intensity of cortical grey matter

-  **-sgm value** image intensity of sub-cortical grey matter

-  **-wm value** image intensity of white matter

-  **-csf value** image intensity of CSF

-  **-path value** image intensity of pathological tissue

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

**Author:** Robert E. Smith (r.smith@brain.org.au)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
