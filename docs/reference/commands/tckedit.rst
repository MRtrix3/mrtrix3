.. _tckedit:

tckedit
===================

Synopsis
--------

Perform various editing operations on track files

Usage
--------

::

    tckedit [ options ]  tracks_in [ tracks_in ... ] tracks_out

-  *tracks_in*: the input track file(s)
-  *tracks_out*: the output track file

Description
-----------

This command can be used to perform various manipulations on track data. This includes: merging data from multiple track files into one; extracting only a finite number of tracks; selecting a subset of tracks based on various criteria, for instance regions of interest.

Note that if multi-threading is used in this command, the ordering of tracks in the output file is unlikely to match the order of the incoming data. If your application explicitly requires that the order of tracks not change, you should run this command with the option -nthreads 0.

Options
-------

Region Of Interest processing options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-include image** specify an inclusion region of interest, as either a binary mask image, or as a sphere using 4 comma-separared values (x,y,z,radius). Streamlines must traverse ALL inclusion regions to be accepted.

-  **-exclude image** specify an exclusion region of interest, as either a binary mask image, or as a sphere using 4 comma-separared values (x,y,z,radius). Streamlines that enter ANY exclude region will be discarded.

-  **-mask image** specify a masking region of interest, as either a binary mask image, or as a sphere using 4 comma-separared values (x,y,z,radius). If defined, streamlines exiting the mask will be truncated.

Streamline length threshold options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-maxlength value** set the maximum length of any streamline in mm

-  **-minlength value** set the minimum length of any streamline in mm

Streamline count truncation options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-number count** set the desired number of selected streamlines to be propagated to the output file

-  **-skip count** omit this number of selected streamlines before commencing writing to the output file

Thresholds pertaining to per-streamline weighting
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-maxweight value** set the maximum weight of any streamline

-  **-minweight value** set the minimum weight of any streamline

Other options specific to tckedit
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-inverse** output the inverse selection of streamlines based on the criteria provided, i.e. only those streamlines that fail at least one criterion will be written to file.

-  **-ends_only** only test the ends of each streamline against the provided include/exclude ROIs

Options for handling streamline weights
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-tck_weights_in path** specify a text scalar file containing the streamline weights

-  **-tck_weights_out path** specify the path for an output text scalar file containing streamline weights

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

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.


