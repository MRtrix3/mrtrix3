.. _tckgen:

tckgen
===========

Synopsis
--------

::

    tckgen [ options ]  source tracks

-  *source*: the image containing the source data. The type of data depends on the algorithm used:- FACT: the directions file (each triplet of volumes is the X,Y,Z direction of a fibre population).- iFOD1/2, Nulldist2 & SD_Stream: the SH image resulting from CSD.- Nulldist1 & SeedTest: any image (will not be used).- Tensor_Det / Tensor_Prob: the DWI image.
-  *tracks*: the output file containing the tracks generated.

Description
-----------

perform streamlines tractography.

Options
-------

-  **-algorithm name** specify the tractography algorithm to use. Valid choices are: FACT, iFOD1, iFOD2, Nulldist1, Nulldist2, SD_Stream, Seedtest, Tensor_Det, Tensor_Prob (default: iFOD2).

Region Of Interest processing options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-include spec** specify an inclusion region of interest, as either a binary mask image, or as a sphere using 4 comma-separared values (x,y,z,radius). Streamlines must traverse ALL inclusion regions to be accepted.

-  **-exclude spec** specify an exclusion region of interest, as either a binary mask image, or as a sphere using 4 comma-separared values (x,y,z,radius). Streamlines that enter ANY exclude region will be discarded.

-  **-mask spec** specify a masking region of interest, as either a binary mask image, or as a sphere using 4 comma-separared values (x,y,z,radius). If defined, streamlines exiting the mask will be truncated.

Streamlines tractography options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-step size** set the step size of the algorithm in mm (default is 0.1 x voxelsize; for iFOD2: 0.5 x voxelsize).

-  **-angle theta** set the maximum angle between successive steps (default is 90deg x stepsize / voxelsize).

-  **-number tracks** set the desired number of tracks. The program will continue to generate tracks until this number of tracks have been selected and written to the output file; set to 0 to ignore limit.

-  **-maxnum tracks** set the maximum number of tracks to generate. The program will not generate more tracks than this number, even if the desired number of tracks hasn't yet been reached (default is 100 x number); set to 0 to ignore limit.

-  **-maxlength value** set the maximum length of any track in mm (default is 100 x voxelsize).

-  **-minlength value** set the minimum length of any track in mm (default is 5 x voxelsize without ACT, 2 x voxelsize with ACT).

-  **-cutoff value** set the FA or FOD amplitude cutoff for terminating tracks (default is 0.1).

-  **-initcutoff value** set the minimum FA or FOD amplitude for initiating tracks (default is the same as the normal cutoff).

-  **-trials number** set the maximum number of sampling trials at each point (only used for probabilistic tracking).

-  **-unidirectional** track from the seed point in one direction only (default is to track in both directions).

-  **-initdirection dir** specify an initial direction for the tracking (this should be supplied as a vector of 3 comma-separated values.

-  **-noprecomputed** do NOT pre-compute legendre polynomial values. Warning: this will slow down the algorithm by a factor of approximately 4.

-  **-power value** raise the FOD to the power specified (default is 1/nsamples).

-  **-samples number** set the number of FOD samples to take per step for the 2nd order (iFOD2) method (Default: 4).

-  **-rk4** use 4th-order Runge-Kutta integration (slower, but eliminates curvature overshoot in 1st-order deterministic methods)

-  **-stop** stop propagating a streamline once it has traversed all include regions

-  **-downsample factor** downsample the generated streamlines to reduce output file size (default is (samples-1) for iFOD2, no downsampling for all other algorithms)

Anatomically-Constrained Tractography options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-act image** use the Anatomically-Constrained Tractography framework during tracking;provided image must be in the 5TT (five-tissue-type) format

-  **-backtrack** allow tracks to be truncated and re-tracked if a poor structural termination is encountered

-  **-crop_at_gmwmi** crop streamline endpoints more precisely as they cross the GM-WM interface

Tractography seeding options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-seed_sphere spec** spherical seed as four comma-separated values (XYZ position and radius)

-  **-seed_image image** seed streamlines entirely at random within a mask image (this is the same behaviour as the streamline seeding in MRtrix 0.2)

-  **-seed_random_per_voxel image num_per_voxel** seed a fixed number of streamlines per voxel in a mask image; random placement of seeds in each voxel

-  **-seed_grid_per_voxel image grid_size** seed a fixed number of streamlines per voxel in a mask image; place seeds on a 3D mesh grid (grid_size argument is per axis; so a grid_size of 3 results in 27 seeds per voxel)

-  **-seed_rejection image** seed from an image using rejection sampling (higher values = more probable to seed from)

-  **-seed_gmwmi image** seed from the grey matter - white matter interface (only valid if using ACT framework). Input image should be a 3D seeding volume; seeds drawn within this image will be optimised to the interface using the 5TT image provided using the -act option.

-  **-seed_dynamic fod_image** determine seed points dynamically using the SIFT model (must not provide any other seeding mechanism). Note that while this seeding mechanism improves the distribution of reconstructed streamlines density, it should NOT be used as a substitute for the SIFT method itself.

-  **-max_seed_attempts count** set the maximum number of times that the tracking algorithm should attempt to find an appropriate tracking direction from a given seed point

-  **-output_seeds path** output the seed location of all successful streamlines to a file

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad encoding** specify the diffusion-weighted gradient scheme used in the acquisition. The program will normally attempt to use the encoding stored in the image header. This should be supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2.

-  **-fslgrad bvecs bvals** specify the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format.

-  **-bvalue_scaling mode** specifies whether the b-values should be scaled by the square of the corresponding DW gradient norm, as often required for multi-shell or DSI DW acquisition schemes. The default action can also be set in the MRtrix config file, under the BValueScaling entry. Valid choices are yes/no, true/false, 0/1 (default: true).

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

References
^^^^^^^^^^

References based on streamlines algorithm used:

* FACT:Mori, S.; Crain, B. J.; Chacko, V. P. & van Zijl, P. C. M. Three-dimensional tracking of axonal projections in the brain by magnetic resonance imaging. Annals of Neurology, 1999, 45, 265-269

* iFOD1 or SD_STREAM:Tournier, J.-D.; Calamante, F. & Connelly, A. MRtrix: Diffusion tractography in crossing fiber regions. Int. J. Imaging Syst. Technol., 2012, 22, 53-66

* iFOD2:Tournier, J.-D.; Calamante, F. & Connelly, A. Improved probabilistic streamlines tractography by 2nd order integration over fibre orientation distributions. Proceedings of the International Society for Magnetic Resonance in Medicine, 2010, 1670

* Nulldist1 / Nulldist2:Morris, D. M.; Embleton, K. V. & Parker, G. J. Probabilistic fibre tracking: Differentiation of connections from chance events. NeuroImage, 2008, 42, 1329-1339

* Tensor_Det:Basser, P. J.; Pajevic, S.; Pierpaoli, C.; Duda, J. & Aldroubi, A. In vivo fiber tractography using DT-MRI data. Magnetic Resonance in Medicine, 2000, 44, 625-632

* Tensor_Prob:Jones, D. Tractography Gone Wild: Probabilistic Fibre Tracking Using the Wild Bootstrap With Diffusion Tensor MRI. IEEE Transactions on Medical Imaging, 2008, 27, 1268-1274

References based on command-line options:

* -rk4:Basser, P. J.; Pajevic, S.; Pierpaoli, C.; Duda, J. & Aldroubi, A. In vivo fiber tractography using DT-MRI data. Magnetic Resonance in Medicine, 2000, 44, 625-632

* -act, -backtrack, -seed_gmwmi:Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. NeuroImage, 2012, 62, 1924-1938

* -seed_dynamic:Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. SIFT2: Enabling dense quantitative assessment of brain white matter connectivity using streamlines tractography. NeuroImage, 2015, 119, 338-351

--------------



**Author:** J-Donald Tournier (jdtournier@gmail.com) and Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2008-2017 the MRtrix3 contributors

This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0/.

MRtrix is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

For more details, see http://www.mrtrix.org/.

