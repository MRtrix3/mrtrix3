sh2amp
===========

Synopsis
--------

::

    sh2amp [ options ]  input directions output

-  *input*: the input image consisting of spherical harmonic (SH)
   coefficients.
-  *directions*: the list of directions along which the SH functions
   will be sampled, generated using the dirgen command
-  *output*: the output image consisting of the amplitude of the SH
   functions along the specified directions.

Description
-----------

Evaluate the amplitude of an image of spherical harmonic functions along
the specified directions

Options
-------

-  **-gradient** assume input directions are supplied as a gradient
   encoding file

-  **-nonnegative** cap all negative amplitudes to zero

Stride options
^^^^^^^^^^^^^^

-  **-stride spec** specify the strides of the output data in memory,
   as a comma-separated list. The actual strides produced will depend on
   whether the output image format can support it.

Data type options
^^^^^^^^^^^^^^^^^

-  **-datatype spec** specify output image data type. Valid choices
   are: float32, float32le, float32be, float64, float64le, float64be,
   int64, uint64, int64le, uint64le, int64be, uint64be, int32, uint32,
   int32le, uint32le, int32be, uint32be, int16, uint16, int16le,
   uint16le, int16be, uint16be, cfloat32, cfloat32le, cfloat32be,
   cfloat64, cfloat64le, cfloat64be, int8, uint8, bit.

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
