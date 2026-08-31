.. _tckfilter:

tckfilter
===================

Synopsis
--------

Perform a smoothing operation on streamline-wise values

Usage
--------

::

    tckfilter [ options ]  input output matrix

-  *input*: provides a txt file containing the streamline-wise data to be smoothed
-  *output*: provides a txt file containing the per streamline smoothed data
-  *matrix*: provides a streamline-streamline similarity matrix folder

Description
-----------

This command smooths a per-streamline data file based on streamline similarity. For each streamline, a new value is computed as a weighted average of the values of its similar streamlines. Smoothing can be modulated by a similarity threshold, and/or a decay rate.

Options
-------

-  **-tck_weights_in value** a txt file containing per-track weights

-  **-threshold value** a threshold to define the smoothing extent (default: 0)

-  **-decay_rate value** a decay rate to modulate the smoothing effect (default: 5)

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


