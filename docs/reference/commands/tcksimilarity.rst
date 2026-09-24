.. _tcksimilarity:

tcksimilarity
===================

Synopsis
--------

Generate a streamline-streamline similarity matrix

Usage
--------

::

    tcksimilarity [ options ]  tracks template matrix

-  *tracks*: the tracks the similarity is computed on
-  *template*: an image file to be used as a template for the shared volume 
-  *matrix*: the output streamline-streamline similarity matrix directory path

Description
-----------

This command will generate a directory containing three images, which encode the streamline-streamline similarity matrix.

Options
-------

Options that influence generation of the similarity matrix
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-threshold value** a threshold for the streamline-streamline similarity to determine the the similarity values to be included in the matrix (default: 0.1)

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

Zanoni, S.; Lv, J.; Smith, R. E. & Calamante, F. Streamline-Based Analysis: A novel framework for tractogram-driven streamline-wise statistical analysis. Proceedings of the International Society for Magnetic Resonance in Medicine, 2025, 4781

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Simone Zanoni (simone.zanoni@sydney.edu.au)

**Copyright:** Copyright (c) 2008-2026 the MRtrix3 contributors.

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


