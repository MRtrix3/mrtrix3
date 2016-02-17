fixel2voxel
===========

Synopsis
--------

::

    fixel2voxel [ options ]  fixel_in operation image_out

-  *fixel_in*: the input sparse fixel image.
-  *operation*: the operation to apply, one of: mean, sum, product, rms,
   var, std, min, max, absmax, magmax, count, complexity, sf, dec_unit,
   dec_scaled, split_size, split_value, split_dir.
-  *image_out*: the output scalar image.

Description
-----------

convert a fixel-based sparse-data image into some form of scalar image.
This could be: - Some statistic computed across all fixel values within
a voxel: mean, sum, product, rms, var, std, min, max, absmax, magmax-
The number of fixels in each voxel: count- Some measure of
crossing-fibre organisation: complexity, sf ('single-fibre')- A 4D
directionally-encoded colour image: dec_unit, dec_scaled- A 4D scalar
image with one 3D volume per fixel: split_size, split_value- A 4D
image with three 3D volumes per fixel direction: split_dir

Options
-------

-  **-weighted** weight the contribution of each fixel to the per-voxel
   result according to its volume (note that this option is not
   applicable for all operations, and should be avoided if the value
   stored in the fixel image is itself the estimated fibre volume)

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

-  Reference for 'complexity' operation:Riffert, T. W.; Schreiber, J.;
   Anwander, A. & Knosche, T. R. Beyond Fractional Anisotropy:
   Extraction of bundle-specific structural metrics from crossing fibre
   models. NeuroImage, 2014 (in press)

--------------

MRtrix new_syntax-1436-ge228c30b, built Feb 17 2016

**Author:** J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (C) 2008 Brain Research Institute, Melbourne,
Australia. This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
