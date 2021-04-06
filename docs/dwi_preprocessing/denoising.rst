DWI denoising
=============

MRtrix includes the command ``dwidenoise``, which implements dMRI noise level 
estimation and denoising based on random matrix theory. The method exploits 
data redundancy in the patch-level PCA domain ([Veraart2016a]_, [Veraart2016b]_ 
and [CorderoGrande2019]_). The method uses the prior knowledge that the 
eigenspectrum of random covariance matrices is described by the universal 
Marchenko-Pastur (MP) distribution.


Recommended use
---------------

Image denoising must be performed as the first step of the image-processing 
pipeline. Interpolation or smoothing in other processing steps, such as motion 
and distortion correction, may alter the noise characteristics and thus 
violate the assumptions upon which MP-PCA is based.

Typical use will be:

::
    
    dwidenoise dwi.mif out.mif -noise noise.mif
  
where ``dwi.mif`` contains the raw input DWI image, ``out.mif`` is the denoised
DWI output, and ``noise.mif`` is the estimated spatially-varying noise level.

We always recommend eyeballing the residuals, i.e. out - in, as part of the 
quality control. The lack of anatomy in the residual maps is a marker of 
accuracy and signal-preservation during denoising. The residuals can be easily
obtained with

::
    
    mrcalc dwi.mif out.mif -subtract res.mif
    mrview res.mif


Advanced options
----------------

Patch size
^^^^^^^^^^

The noise level in MRI is spatially varying, due to the proximity of the coil 
elements and parallel imaging. Noise level estimation and denoising therefore 
operates in image patches around each voxel, where the noise can be assumed to 
be approximately homoscedastic. The patch size can be chosen by the user with
the option ``-extent``. For maximal SNR gain (when using Exp2, see below) we 
suggest to choose :math:`N \approx M`, where :math:`M` is the no. DW volumes 
and :math:`N` is the number of kernel elements. However, larger kernels also 
extend the required run time, so in large datasets it might be beneficial to 
select smaller sliding kernels. By default, the command will select the smallest 
isotropic patch size that exceeds the number of DW images in the input data, 
e.g., 5x5x5 for data with <= 125 DWI volumes, 7x7x7 for data with <= 343 DWI 
volumes, etc.



Noise level estimation
^^^^^^^^^^^^^^^^^^^^^^

The noise level in each patch is experimentally estimated from the eigenvalue 
spectrum of the local data matrix. Assuming :math:`M<N`, :math:`P` signal-carying 
components (also estimated), and :math:`M-P` noise components, the squared noise 
level is estimated as:

.. math::
   \sigma^2 = \frac{\lambda_{P+1}-\lambda_M}{4\sqrt{\gamma}}

where :math:`\lambda_i` are the eigenvalues of the covaliance matrix, sorted in 
decreasing order, and :math:`\gamma` is the matrix ratio.

`dwidenoise` implements two different versions of this estimator, based on a 
different definition of the matrix ratio :math:`\gamma`:

- **Exp1** uses the definition used in the original papers [Veraart2016a]_ and 
  [Veraart2016b]_, namely :math:`\gamma = (M-P)/N`.

- **Exp2** uses :math:`\gamma = (M-P)/(N-P)` instead, which was shown in 
  [CorderoGrande2019]_ to improve the estimation. This is now the default.


Complex data
^^^^^^^^^^^^

Note that `dwidenoise` does not correct for non-Gaussian noise biases present 
in magnitude-reconstructed MRI images. Including MRI phase images can reduce 
such Rician bias [CorderoGrande2019]_, and the command now supports complex 
input data. To this end, users can run:

::
     
     mrcalc dwi_magnitude.mif dwi_phase_rad.mif -polar dwi_complex.mif
     dwidenoise dwi_complex.mif out.mif -noise noise.mif
     mrcalc out.mif -abs out_magnitude.mif

Where `dwi_magnitude.mif` and `dwi_phase_rad.mif` are the magnitude and phase 
images respectively. Note that the code above assumes that the phase is scaled 
in radians. 

Whilst using complex data can effectively reduce Rician noise bias, `dwidenoise` 
currently does not account for spatial noise correlations introduced due to 
in-plane accelleration. [CorderoGrande2019]_ addresses these additional effects 
at reconstruction, but this is not implemented in MRtrix3.



