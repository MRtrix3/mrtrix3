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

This command can be used to perform various types of manipulations on track data. A range of such manipulations are demonstrated in the examples provided below.

Example usages
--------------

-   *Concatenate data from multiple track files into one*::

        $ tckedit *.tck all_tracks.tck

    Here the wildcard operator is used to select all files in the current working directory that have the .tck filetype suffix; but input files can equivalently be specified one at a time explicitly.

-   *Extract a reduced number of streamlines*::

        $ tckedit in_many.tck out_few.tck -number 1k -skip 500

    The number of streamlines requested would typically be less than the number of streamlines in the input track file(s); if it is instead greater, then the command will issue a warning upon completion. By default the streamlines for the output file are extracted from the start of the input file(s); in this example the command is instead instructed to skip the first 500 streamlines, and write to the output file streamlines 501-1500.

-   *Extract streamlines based on selection criteria*::

        $ tckedit in.tck out.tck -include ROI1.mif -include ROI2.mif -minlength 25

    Multiple criteria can be added in a single invocation of tckedit, and a streamline must satisfy all criteria imposed in order to be written to the output file. Note that both -include and -exclude options can be specified multiple times to provide multiple waypoints / exclusion masks.

-   *Select only those streamline vertices within a mask*::

        $ tckedit in.tck cropped.tck -mask mask.mif

    The -mask option is applied to each streamline vertex independently, rather than to each streamline, retaining only those streamline vertices within the mask. As such, use of this option may result in a greater number of output streamlines than input streamlines, as a single input streamline may have the vertices at either endpoint retained but some vertices at its midpoint removed, effectively cutting one long streamline into multiple shorter streamlines.

Options
-------

Region Of Interest processing options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-include spec** *(multiple uses permitted)* specify an inclusion region of interest, as either a binary mask image, or as a sphere using 4 comma-separared values (x,y,z,radius). Streamlines must traverse ALL inclusion regions to be accepted.

-  **-include_ordered image** *(multiple uses permitted)* specify an inclusion region of interest, as either a binary mask image, or as a sphere using 4 comma-separared values (x,y,z,radius). Streamlines must traverse ALL inclusion_ordered regions in the order they are specified in order to be accepted.

-  **-exclude spec** *(multiple uses permitted)* specify an exclusion region of interest, as either a binary mask image, or as a sphere using 4 comma-separared values (x,y,z,radius). Streamlines that enter ANY exclude region will be discarded.

-  **-mask spec** *(multiple uses permitted)* specify a masking region of interest, as either a binary mask image, or as a sphere using 4 comma-separared values (x,y,z,radius). If defined, streamlines exiting the mask will be truncated.

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

-  **-inverse** output the inverse selection of streamlines based on the criteria provided; i.e. only those streamlines that fail at least one selection criterion, and/or vertices that are outside masks if provided, will be written to file

-  **-ends_only** only test the ends of each streamline against the provided include/exclude ROIs

Options for handling streamline weights
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-tck_weights_in path** specify a text scalar file containing the streamline weights

-  **-tck_weights_out path** specify the path for an output text scalar file containing streamline weights

Standard options
^^^^^^^^^^^^^^^^

-  **-info** display information messages.

-  **-quiet** do not display information messages or progress status; alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.

-  **-debug** display debugging messages.

-  **-force** force overwrite of output files (caution: using the same file as input and output might cause unexpected behaviour).

-  **-nthreads number** use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).

-  **-config key value** *(multiple uses permitted)* temporarily set the value of an MRtrix config file entry.

-  **-help** display this information page and exit.

-  **-version** display version information and exit.

References
^^^^^^^^^^

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2023 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Covered Software is provided under this License on an "as is"
basis, without warranty of any kind, either expressed, implied, or
statutory, including, without limitation, warranties that the
Covered Software is free of defects, merchantable, fit for a
particular purpose or non-infringing.
See the Mozilla Public License v. 2.0 for more details.

For more details, see http://www.mrtrix.org/.


