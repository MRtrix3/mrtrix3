DWI denoising
=============

MRtrix now includes a new command ``dwidenoise`` which implements DWI data
denoising and noise map estimation by exploiting data redundancy in the PCA 
domain (`Veraart et al., 2016a, 2016b <#references>`__). The method uses the 
prior knowledge that the eigenspectrum of random covariance matrices is 
described by the universal Marchenko Pastur distribution.

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

The kernel size, default 5x5x5, can be chosen by the user (option: ``-extent``). 
For maximal SNR gain we suggest to choose N>M for which M is typically the 
number of DW images in the data (single or multi-shell), where N is the 
number of kernel elements. However, in case of spatially varying noise, it 
might be beneficial to select smaller sliding kernels, e.g. N~M, to balance 
between precision, accuracy, and resolution of the noise map.

Note that this function does not correct for non-Gaussian noise biases yet.

References
----------

1. J. Veraart, E. Fieremans, and D.S. Novikov *Diffusion MRI noise mapping 
   using random matrix theory.* Magn. Res. Med., early view (2016), 
   `doi: 10.1002/mrm.26059 <http://dx.doi.org/10.1002/mrm.26059>`__

2. J. Veraart, D.S. Novikov, J. Sijbers, and E. Fieremans *Denoising of 
   diffusion MRI data using Random Matrix Theory.* ISMRM, 24 (2016), p. 1047

