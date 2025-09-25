.. _dwirecon:

dwirecon
===================

Synopsis
--------

Perform reconstruction of DWI data from an input DWI series

Usage
--------

::

    dwirecon [ options ]  input operation output

-  *input*: the input DWI series
-  *operation*: the way in which output DWIs will be reconstructed; one of: combine_pairs, combine_predicted
-  *output*: the output DWI series

Description
-----------

This command provides a range of mechanisms by which to reconstruct estimated DWI data given a set of input DWI data and possibly other information about the image acquisition and/or reconstruction process. The operation that is appropriate for a given workflow is entirely dependent on the context of the details of that workflow and how the image data were acquired. Each operation available is described in further detail below.

The "combine_pairs" operation is applicable in the scenario where the DWI acquisition involves acquiring the same diffusion gradient table twice, with the direction of phase encoding reversed in the second acquisition. It is a requirement in this case that the total readout time be equivalent between the two series; that is, they vary based only on the direction of phase encoding, not the speed.The purpose of this command in that context is to take as input the full set of volumes (ie. from both phase encoding directions), find those pairs of DWI volumes with equivalent diffusion sensitisation but opposite phase encoding direction, and explicitly combine each pair into a single output volume, where the contribution of each image in the pair to the output image intensity is modulated by the relative Jacobian determinants of the two distorted images: out = ((in_1 * jacdet_1^2) + (in_2 * jacdet_2^2)) / (jacdet_1^2 + jacdet_2^2).

The "combine_predicted" operation is intended for DWI acquisition designs where the diffusion gradient table is split between different phase encoding directions. Here, where there is greater uncertainty in what the DWI signal should look like due to susceptibility-driven signal compression in the acquired image data, the reconstructed image will be moreso influenced by the signal intensity that is estimated from those volumes with different phase encoding that did not experience such compression. The output signal intensity is determined by the expression: out = (weight * empirical) + ((1.0 - weight) * predicted), where weight = max(0, min(1, jacdet^exponent)); in this way, where the determinant of the Jacobian for a volume is 1 or greater (ie. signal was expanded in the acquired image data) the empirical intensity is preserved exactly, whereas where it is less than 1 (ie. signal was compressed in the acquired image data, leading to a loss of spatial contrast), the empirical data are aggregated with that predicted from groups of volumes with alternative phase encoding directions, with the relative contributions influenced by the value of command-line option -exponent (which has a default value of 1.0).

Options
-------

Options common to multiple operations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-field image** provide a B0 field offset image in Hz

Options specific to "combine_pairs" operation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-pairs_in file** provide a text file specifying the volume indices that should be considered paired and therefore combined

-  **-pairs_out file** output a text file encoding which input volumes were combined in production of the output volumes

Options specific to "combine_predicted" operation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-lmax value** set the maximal spherical harmonic degrees to use (one for each b-value) during signal reconstruction

-  **-exponent value** set the exponent modulating relative contributions between empirical and predicted signal (see Description)

Options for importing phase-encode tables
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-import_pe_table file** import a phase-encoding table from file

-  **-import_pe_topup file** import a phase-encoding table intended for FSL topup from file

-  **-import_pe_eddy config indices** import phase-encoding information from an EDDY-style config / index file pair

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-fslgrad bvecs bvals** Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

DW gradient table export options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-export_grad_mrtrix path** export the diffusion-weighted gradient table to file in MRtrix format

-  **-export_grad_fsl bvecs_path bvals_path** export the diffusion-weighted gradient table to files in FSL (bvecs / bvals) format

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

**Copyright:** Copyright (c) 2008-2025 the MRtrix3 contributors.

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


