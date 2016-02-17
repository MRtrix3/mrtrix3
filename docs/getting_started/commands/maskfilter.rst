maskfilter
===========

Synopsis
--------

::

    maskfilter [ options ]  input filter output

-  *input*: the input image.
-  *filter*: the type of filter to be applied (connect, dilate, erode,
   median)
-  *output*: the output image.

Description
-----------

Perform filtering operations on 3D / 4D mask images.

The available filters are: connect, dilate, erode, median.

Each filter has its own unique set of optional parameters.

Options
-------

Options for connected-component filter
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-axes axes** specify which axes should be included in the
   connected components. By default only the first 3 axes are included.
   The axes should be provided as a comma-separated list of values.

-  **-largest** only retain the largest connected component

-  **-connectivity** use 26 neighbourhood connectivity (Default: 6)

Options for dilate / erode filters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-npass value** the number of times to repeatedly apply the filter

Options for median filter
^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-extent voxels** specify the extent (width) of kernel size in
   voxels. This can be specified either as a single value to be used for
   all axes, or as a comma-separated list of the extent for each axis.
   The default is 3x3x3.

Stride options
^^^^^^^^^^^^^^

-  **-stride spec** specify the strides of the output data in memory,
   as a comma-separated list. The actual strides produced will depend on
   whether the output image format can support it.

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

**Author:** Robert E. Smith (r.smith@brain.org.au), David Raffelt
(david.raffelt@florey.edu.au) and J-Donald Tournier
(jdtournier@gmail.com)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
