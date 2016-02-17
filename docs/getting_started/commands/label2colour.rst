label2colour
===========

Synopsis
--------

::

    label2colour [ options ]  nodes_in colour_out

-  *nodes_in*: the input node parcellation image
-  *colour_out*: the output colour image

Description
-----------

convert a parcellated image (where values are node indices) into a
colour image (many software packages handle this colouring internally
within their viewer program; this binary explicitly converts a
parcellation image into a colour image that should be viewable in any
software)

Options
-------

Options for importing information from parcellation lookup tables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-lut_basic path** get information from a basic lookup table
   consisting of index / name pairs

-  **-lut_freesurfer path** get information from a FreeSurfer lookup
   table (typically "FreeSurferColorLUT.txt")

-  **-lut_aal path** get information from the AAL lookup table
   (typically "ROI_MNI_V4.txt")

-  **-lut_itksnap path** get information from an ITK-SNAP lookup table
   (this includes the IIT atlas file "LUT_GM.txt")

-  **-config file** If the input parcellation image was created using
   labelconfig, provide the connectome config file used so that the node
   indices are converted correctly

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
