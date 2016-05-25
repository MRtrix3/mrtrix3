labelsgmfix
===========

Synopsis
--------

    labelsgmfix [ options ] parc t1 config output

- *parc*: The input FreeSurfer parcellation image
- *t1*: The T1 image to be provided to FIRST
- *config*: Either the FreeSurfer lookup table (if the input parcellation image is directly from FreeSurfer), or the connectome configuration file (if the parcellation image has been modified using the labelconfig command)
- *output*: The output parcellation image

Description
-----------

In a FreeSurfer parcellation image, replace the sub-cortical grey matter structure delineations using FSL FIRST

Options
-------

- **-premasked** Indicate that brain masking has been applied to the T1 input image

- **-sgm_amyg_hipp** Consider the amygdalae and hippocampi as sub-cortical grey matter structures, and also replace their estimates with those from FIRST

Standard options
^^^^^^^^^^^^^^^^


- **-continue <TempDir> <LastFile>** Continue the script from a previous execution; must provide the temporary directory path, and the name of the last successfully-generated file

- **-force** Force overwrite of output files if pre-existing

- **-help** Display help information for the script

- **-nocleanup** Do not delete temporary directory at script completion

- **-nthreads number** Use this number of threads in MRtrix multi-threaded applications (0 disables multi-threading)

- **-tempdir /path/to/tmp/** Manually specify the path in which to generate the temporary directory

- **-quiet** Suppress all console output during script execution

- **-verbose** Display additional information for every command invoked

References
^^^^^^^^^^

first:
Patenaude, B.; Smith, S. M.; Kennedy, D. N. & Jenkinson, M. A Bayesian model of shape and appearance for subcortical brain segmentation. NeuroImage, 2011, 56, 907-922


FSL:
Smith, S. M.; Jenkinson, M.; Woolrich, M. W.; Beckmann, C. F.; Behrens, T. E.; Johansen-Berg, H.; Bannister, P. R.; De Luca, M.; Drobnjak, I.; Flitney, D. E.; Niazy, R. K.; Saunders, J.; Vickers, J.; Zhang, Y.; De Stefano, N.; Brady, J. M. & Matthews, P. M. Advances in functional and structural MR image analysis and implementation as FSL. NeuroImage, 2004, 23, S208-S219


SIFT_followup:
Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. The effects of SIFT on the reproducibility and biological accuracy of the structural connectome. NeuroImage, 2015, 104, 253-265



---

**Author:** Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** 
Copyright (c) 2008-2016 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public 
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/

MRtrix is distributed in the hope that it will be useful, 
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see www.mrtrix.org
