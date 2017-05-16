.. _dwiintensitynorm:

dwiintensitynorm
================

Synopsis
--------

Performs a global DWI intensity normalisation on a group of subjects using the median b=0 white matter value as the reference

Usage
--------

::

    dwiintensitynorm input_dir mask_dir output_dir fa_template wm_mask [ options ]

-  *input_dir*: The input directory containing all DWI images
-  *mask_dir*: Input directory containing brain masks, corresponding to one per input image (with the same file name prefix)
-  *output_dir*: The output directory containing all of the intensity normalised DWI images
-  *fa_template*: The output population specific FA template, which is threshold to estimate a white matter mask
-  *wm_mask*: The output white matter mask (in template space), used to estimate the median b=0 white matter value for normalisation

Description
-----------

The white matter mask is estimated from a population average FA template then warped back to each subject to perform the intensity normalisation. Note that bias field correction should be performed prior to this step.

Options
-------

Options for the dwiintensitynorm script
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- **-fa_threshold** The threshold applied to the Fractional Anisotropy group template used to derive an approximate white matter mask

Standard options
^^^^^^^^^^^^^^^^

- **-continue <TempDir> <LastFile>** Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file

- **-force** Force overwrite of output files if pre-existing

- **-help** Display help information for the script

- **-nocleanup** Do not delete temporary files during script, or temporary directory at script completion

- **-nthreads number** Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)

- **-tempdir /path/to/tmp/** Manually specify the path in which to generate the temporary directory

- **-quiet** Suppress all console output during script execution

- **-info** Display additional information and progress for every command invoked

- **-debug** Display additional debugging information over and above the output of -info

--------------



**Author:** David Raffelt (david.raffelt@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

