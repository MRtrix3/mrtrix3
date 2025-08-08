.. _dwi2noise:

dwi2noise
===================

Synopsis
--------

Noise level estimation using Marchenko-Pastur PCA

Usage
--------

::

    dwi2noise [ options ]  dwi noise

-  *dwi*: the input diffusion-weighted image
-  *noise*: the output estimated noise level map

Description
-----------

DWI data noise map estimation by interrogating data redundancy in the PCA domain using the prior knowledge that the eigenspectrum of random covariance matrices is described by the universal Marchenko-Pastur (MP) distribution. Fitting the MP distribution to the spectrum of patch-wise signal matrices hence provides an estimator of the noise level 'sigma'.

Unlike the MRtrix3 command dwidenoise, this command does not generate a denoised version of the input image series; its primary output is instead a map of the estimated noise level. While this can also be obtained from the dwidenoise command using option -noise_out, using instead the dwi2noise command gives the ability to obtain a noise map to which filtering can be applied, which can then be utilised for the actual image series denoising, without generating an unwanted intermiedate denoised image series.

Important note: noise level estimation should only be performed as the first step of an image processing pipeline. The routine is invalid if interpolation or smoothing has been applied to the data prior to denoising.

Note that on complex input data, the output will be the total noise level across real and imaginary channels, so a scale factor sqrt(2) applies.

If the input data are of complex type, then a smooth non-linear phase will be demodulated removed from each k-space prior to PCA. In the absence of metadata indicating otherwise, it is inferred that the first two axes correspond to acquired slices, and different slices / volumes will be demodulated individually; this behaviour can be modified using the -demod_axes option. A strictly linear phase term can instead be regressed from each k-space, similarly to performed in Cordero-Grande et al. 2019, by specifying -demodulate linear.

The sliding spatial window behaves differently at the edges of the image FoV depending on the shape / size selected for that window. The default behaviour is to use a spherical kernel centred at the voxel of interest, whose size is some multiple of the number of input volumes; where some such voxels lie outside of the image FoV, the radius of the kernel will be increased until the requisite number of voxels are used. For a spherical kernel of a fixed radius, no such expansion will occur, and so for voxels near the image edge a reduced number of voxels will be present in the kernel. For a cuboid kernel, the centre of the kernel will be offset from the voxel being processed such that the entire volume of the kernel resides within the image FoV.

The size of the default spherical kernel is set to select a number of voxels that is 1.0 / 0.85 ~ 1.18 times the number of volumes in the input series. If a cuboid kernel is requested, but the -extent option is not specified, the command will select the smallest isotropic patch size that exceeds the number of DW images in the input data; e.g., 5x5x5 for data with <= 125 DWI volumes, 7x7x7 for data with <= 343 DWI volumes, etc.

Permissible sizes for the cuboid kernel depend on the subsampling factor. If no subsampling is performed, or the subsampling factor is odd, then the extent(s) of the kernel must be odd, such that a unique voxel lies at the very centre of each kernel. If however an even subsampling factor is used, then the extent(s) of the kernel must be even, reflecting the fact that it is a voxel corner that resides at the centre of the kernel.In either case, if the extent is specified manually, the user can either provide a single integer---which will determine the number of voxels in the kernel across all three spatial axes---or a comma-separated list of three integers,individually defining the number of voxels in the kernel for all three spatial axes.

Options
-------

Options for modifying PCA computations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-datatype float32/float64** Datatype for the eigenvalue decomposition (single or double precision). For complex input data, this will select complex float32 or complex float64 datatypes.

-  **-estimator algorithm** Select the noise level estimator (default = Exp2), either:  |br|
   * Exp1: the original estimator used in Veraart et al. (2016);  |br|
   * Exp2: the improved estimator introduced in Cordero-Grande et al. (2019);  |br|
   * Med: estimate based on the median eigenvalue as in Gavish and Donohue (2014);  |br|
   * MRM2022: the alternative estimator introduced in Olesen et al. (2022).  |br|
   Operation will be bypassed if -noise_in or -fixed_rank are specified

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

DW gradient table import options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-grad file** Provide the diffusion-weighted gradient scheme used in the acquisition in a text file. This should be supplied as a 4xN text file with each line in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units of s/mm^2. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

-  **-fslgrad bvecs bvals** Provide the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format files. If a diffusion gradient scheme is present in the input image header, the data provided with this option will be instead used.

DW gradient table export options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-export_grad_mrtrix path** export the diffusion-weighted gradient table to file in MRtrix format

-  **-export_grad_fsl bvecs_path bvals_path** export the diffusion-weighted gradient table to files in FSL (bvecs / bvals) format

Options for exporting additional data regarding PCA behaviour
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-rank image** The signal rank estimated for each denoising patch

Options for debugging the operation of sliding window kernels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

-  **-max_dist image** The maximum distance between the centre of the patch and a voxel that was included within that patch

-  **-voxelcount image** The number of voxels that contributed to the PCA for processing of each patch

-  **-patchcount image** The number of unique patches to which an input image voxel contributes

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

Veraart, J.; Fieremans, E. & Novikov, D.S. Diffusion MRI noise mapping using random matrix theory. Magn. Res. Med., 2016, 76(5), 1582-1593, doi: 10.1002/mrm.26059

Cordero-Grande, L.; Christiaens, D.; Hutter, J.; Price, A.N.; Hajnal, J.V. Complex diffusion-weighted image estimation via matrix recovery under general noise models. NeuroImage, 2019, 200, 391-404, doi: 10.1016/j.neuroimage.2019.06.039

* If using -estimator mrm2022: Olesen, J.L.; Ianus, A.; Ostergaard, L.; Shemesh, N.; Jespersen, S.N. Tensor denoising of multidimensional MRI data. Magnetic Resonance in Medicine, 2022, 89(3), 1160-1172

* If using -estimator med: Gavish, M.; Donoho, D.L. The Optimal Hard Threshold for Singular Values is 4/sqrt(3). IEEE Transactions on Information Theory, 2014, 60(8), 5040-5053.

Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; Jeurissen, B.; Yeh, C.-H. & Connelly, A. MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. NeuroImage, 2019, 202, 116137

--------------



**Author:** Daan Christiaens (daan.christiaens@kcl.ac.uk) and Jelle Veraart (jelle.veraart@nyumc.org) and J-Donald Tournier (jdtournier@gmail.com) and Robert E. Smith (robert.smith@florey.edu.au)

**Copyright:** Copyright (c) 2016 New York University, University of Antwerp, and the MRtrix3 contributors 
 
Permission is hereby granted, free of charge, to any non-commercial entity ('Recipient') obtaining a copy of this software and associated documentation files (the 'Software'), to the Software solely for non-commercial research, including the rights to use, copy and modify the Software, subject to the following conditions: 
 
	 1. The above copyright notice and this permission notice shall be included by Recipient in all copies or substantial portions of the Software. 
 
	 2. THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIESOF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BELIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF ORIN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 
	 3. In no event shall NYU be liable for direct, indirect, special, incidental or consequential damages in connection with the Software. Recipient will defend, indemnify and hold NYU harmless from any claims or liability resulting from the use of the Software by recipient. 
 
	 4. Neither anything contained herein nor the delivery of the Software to recipient shall be deemed to grant the Recipient any right or licenses under any patents or patent application owned by NYU. 
 
	 5. The Software may only be used for non-commercial research and may not be used for clinical care. 
 
	 6. Any publication by Recipient of research involving the Software shall cite the references listed below.

