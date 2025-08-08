.. _dwidenoise2:

dwidenoise2
===================

Synopsis
--------

Improved dMRI denoising using Marchenko-Pastur PCA

Usage
--------

::

    dwidenoise2 [ options ]  dwi out

-  *dwi*: the input diffusion-weighted image.
-  *out*: the output denoised DWI image.

Description
-----------

This command performs DWI data denoising, additionally with data-driven noise map estimation if not provided explicitly. The output denoised DWI data is formed based on filtering of eigenvectors of PCA decompositions: for a set of patches each of which consists of a set of proximal voxels, the PCA decomposition is applied, and the DWI signal for those voxels is reconstructed where the contribution of each eigenvector is modulated based on its classification as noise. In many use cases, a threshold that classifies eigenvalues as belonging to signal of interest vs. random thermal noise is based on the prior knowledge that the eigenspectrum of random covariance matrices is described by the universal Marchenko-Pastur (MP) distribution.

This command includes many capabilities absent from the original dwidenoise command. These include: - Multiple sliding window kernel shapes, including a spherical kernel that dilates at image edges to preserve aspect ratio; - A greater number of mechanisms for noise level estimation, including taking a pre-estimated noise map as input; - Preconditioning, including (per-shell) demeaning, phase demodulation (linear or nonlinear), and variance-stabilising transform to compensate for within-patch heteroscedasticity; - Overcomplete local PCA; - Subsampling (performing fewer PCAs than there are input voxels); - Optimal shrinkage of eigenvalues.

Important note: image denoising must be performed as the first step of the image processing pipeline. The routine will not operate correctly if interpolation or smoothing has been applied to the data prior to denoising.

Note that this function does not correct for non-Gaussian noise biases present in magnitude-reconstructed MRI images. If available, including the MRI phase data as part of a complex input image can reduce such non-Gaussian biases.

If the input data are of complex type, then a smooth non-linear phase will be demodulated removed from each k-space prior to PCA. In the absence of metadata indicating otherwise, it is inferred that the first two axes correspond to acquired slices, and different slices / volumes will be demodulated individually; this behaviour can be modified using the -demod_axes option. A strictly linear phase term can instead be regressed from each k-space, similarly to performed in Cordero-Grande et al. 2019, by specifying -demodulate linear.

The sliding spatial window behaves differently at the edges of the image FoV depending on the shape / size selected for that window. The default behaviour is to use a spherical kernel centred at the voxel of interest, whose size is some multiple of the number of input volumes; where some such voxels lie outside of the image FoV, the radius of the kernel will be increased until the requisite number of voxels are used. For a spherical kernel of a fixed radius, no such expansion will occur, and so for voxels near the image edge a reduced number of voxels will be present in the kernel. For a cuboid kernel, the centre of the kernel will be offset from the voxel being processed such that the entire volume of the kernel resides within the image FoV.

The size of the default spherical kernel is set to select a number of voxels that is 1.0 / 0.85 ~ 1.18 times the number of volumes in the input series. If a cuboid kernel is requested, but the -extent option is not specified, the command will select the smallest isotropic patch size that exceeds the number of DW images in the input data; e.g., 5x5x5 for data with <= 125 DWI volumes, 7x7x7 for data with <= 343 DWI volumes, etc.

Permissible sizes for the cuboid kernel depend on the subsampling factor. If no subsampling is performed, or the subsampling factor is odd, then the extent(s) of the kernel must be odd, such that a unique voxel lies at the very centre of each kernel. If however an even subsampling factor is used, then the extent(s) of the kernel must be even, reflecting the fact that it is a voxel corner that resides at the centre of the kernel.In either case, if the extent is specified manually, the user can either provide a single integer---which will determine the number of voxels in the kernel across all three spatial axes---or a comma-separated list of three integers,individually defining the number of voxels in the kernel for all three spatial axes.

By default, optimal value shrinkage based on minimisation of the Frobenius norm will be used to attenuate eigenvectors based on the estimated noise level. Hard truncation of sub-threshold components and inclusion of supra-threshold components---which was the behaviour of the dwidenoise command in version 3.0.x---can be activated using -filter truncate.Alternatively, optimal truncation as described in Gavish and Donoho 2014 can be utilised by specifying -filter optthresh.

-aggregation exclusive corresponds to the behaviour of the dwidenoise command in version 3.0.x, where the output intensities for a given image voxel are determined exclusively from the PCA decomposition where the sliding spatial window is centred at that voxel. In all other use cases, so-called "overcomplete local PCA" is performed, where the intensities for an output image voxel are some combination of all PCA decompositions for which that voxel is included in the local spatial kernel. There are multiple algebraic forms that modulate the weight with which each decomposition contributes with greater or lesser strength toward the output image intensities. The various options are: 'gaussian': A Gaussian distribution with FWHM equal to twice the voxel size, such that decompisitions centred more closely to the output voxel have greater influence; 'invl0': The inverse of the L0 norm (ie. rank) of each decomposition, as used in Manjon et al. 2013; 'rank': The rank of each decomposition, such that high-rank decompositions contribute more strongly to the output intensities regardless of distance between the output voxel and the centre of the decomposition kernel; 'uniform': All decompositions that include the output voxel in the sliding spatial window contribute equally.

Example usages
--------------

-   *To approximately replicate the behaviour of the original dwidenoise command*::

        $ dwidenoise2 DWI.mif out.mif -shape cuboid -subsample 1 -demodulate none -demean none -filter truncate -aggregator exclusive

    While this is neither guaranteed to match exactly the output of the original dwidenoise command nor is it a recommended use case, it may nevertheless be informative in demonstrating those advanced features of dwidenoise2 active by default that must be explicitly disabled in order to approximate that behaviour.

Options
-------

Options for modifying PCA computations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-datatype float32/float64** Datatype for the eigenvalue decomposition (single or double precision). For complex input data, this will select complex float32 or complex float64 datatypes.

Options relating to signal / noise level estimation for denoising
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-estimator algorithm** Select the noise level estimator (default = Exp2), either:  |br|
   * Exp1: the original estimator used in Veraart et al. (2016);  |br|
   * Exp2: the improved estimator introduced in Cordero-Grande et al. (2019);  |br|
   * Med: estimate based on the median eigenvalue as in Gavish and Donohue (2014);  |br|
   * MRM2022: the alternative estimator introduced in Olesen et al. (2022).  |br|
   Operation will be bypassed if -noise_in or -fixed_rank are specified

-  **-noise_in image** import a pre-estimated noise level map for denoising rather than estimating this level from data

-  **-fixed_rank value** set a fixed input signal rank rather than estimating the noise level from the data

Options for controlling the sliding spatial window kernel
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-shape choice** Set the shape of the sliding spatial window. Options are: cuboid,sphere; default: sphere

-  **-radius_mm value** Set an absolute spherical kernel radius in mm

-  **-radius_ratio value** Set the spherical kernel size as a ratio of number of voxels to number of input volumes (default: 1.0/0.85 ~= 1.18)

-  **-extent window** Set the patch size of the cuboid kernel; can be either a single integer or a comma-separated triplet of integers (see Description)

-  **-subsample factor** reduce the number of PCA kernels relative to the number of image voxels; can provide either an integer subsampling factor, or a comma-separated list of three factors; default: 2

Options for preconditioning data prior to PCA
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-demodulate mode** select form of phase demodulation; options are: none,linear,nonlinear (default: nonlinear)

-  **-demod_axes axes** comma-separated list of axis indices along which FFT can be applied for phase demodulation

-  **-demean mode** select method of demeaning prior to PCA; options are: none,shells,all (default: 'shells' if DWI gradient table available, 'all' otherwise)

-  **-vst image** apply a within-patch variance-stabilising transformation based on a pre-estimated noise level map

-  **-preconditioned image** export the preconditioned version of the input image that is the input to PCA

Options that affect reconstruction of the output image series
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-filter choice** Modulate how component contributions are filtered based on the cumulative eigenvalues relative to the noise level; options are: optshrink,optthresh,truncate; default: optshrink (Optimal Shrinkage based on minimisation of the Frobenius norm)

-  **-aggregator choice** Select how the outcomes of multiple PCA outcomes centred at different voxels contribute to the reconstructed DWI signal in each voxel; options are: exclusive,gaussian,invl0,rank,uniform; default: Gaussian

Options for exporting additional data regarding PCA behaviour
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-noise_out image** The output noise map, i.e., the estimated noise level 'sigma' in the data. Note that on complex input data, this will be the total noise level across real and imaginary channels, so a scale factor sqrt(2) applies.

-  **-rank_input image** The signal rank estimated for each denoising patch

-  **-rank_output image** An estimated rank for the output image data, accounting for multi-patch aggregation

Options for debugging the operation of sliding window kernels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-max_dist image** The maximum distance between a voxel and another voxel that was included in the local denoising patch

-  **-voxelcount image** The number of voxels that contributed to the PCA for processing of each voxel

-  **-patchcount image** The number of unique patches to which an image voxel contributes

-  **-sum_aggregation image** The sum of aggregation weights of those patches contributing to each output voxel

-  **-sum_optshrink image** the sum of eigenvector weights computed for the denoising patch centred at each voxel as a result of performing optimal shrinkage

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

Veraart, J.; Novikov, D.S.; Christiaens, D.; Ades-aron, B.; Sijbers, J. & Fieremans, E. Denoising of diffusion MRI using random matrix theory. NeuroImage, 2016, 142, 394-406, doi: 10.1016/j.neuroimage.2016.08.016

Veraart, J.; Fieremans, E. & Novikov, D.S. Diffusion MRI noise mapping using random matrix theory. Magn. Res. Med., 2016, 76(5), 1582-1593, doi: 10.1002/mrm.26059

Cordero-Grande, L.; Christiaens, D.; Hutter, J.; Price, A.N.; Hajnal, J.V. Complex diffusion-weighted image estimation via matrix recovery under general noise models. NeuroImage, 2019, 200, 391-404, doi: 10.1016/j.neuroimage.2019.06.039

* If using -estimator mrm2022: Olesen, J.L.; Ianus, A.; Ostergaard, L.; Shemesh, N.; Jespersen, S.N. Tensor denoising of multidimensional MRI data. Magnetic Resonance in Medicine, 2022, 89(3), 1160-1172

* If using anything other than -aggregation exclusive: Manjon, J.V.; Coupe, P.; Concha, L.; Buades, A.; D. Collins, D.L.; Robles, M. Diffusion Weighted Image Denoising Using Overcomplete Local PCA. PLoS ONE, 2013, 8(9), e73021

* If using -estimator med or -filter optthresh: Gavish, M.; Donoho, D.L.The Optimal Hard Threshold for Singular Values is 4/sqrt(3). IEEE Transactions on Information Theory, 2014, 60(8), 5040-5053.

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Robert E. Smith (robert.smith@florey.edu.au) and Daan Christiaens (daan.christiaens@kcl.ac.uk) and Jelle Veraart (jelle.veraart@nyumc.org) and J-Donald Tournier (jdtournier@gmail.com)

**Copyright:** Copyright (c) 2008-2024 the MRtrix3 contributors.

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


