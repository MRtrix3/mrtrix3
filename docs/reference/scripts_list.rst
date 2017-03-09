
Python scripts provided with MRtrix3
====================================


.. toctree::
   :hidden:
   :glob:

   scripts/*


:ref:`5ttgen` 
  Generate a 5TT image suitable for ACT

:ref:`dwi2response` 
  Estimate response function(s) for spherical deconvolution

:ref:`dwibiascorrect` 
  Perform B1 field inhomogeneity correction for a DWI volume series

:ref:`dwiintensitynorm` 
  Performs a global DWI intensity normalisation on a group of subjects using the median b=0 white matter value as the reference. The white matter mask is estimated from a population average FA template then warped back to each subject to perform the intensity normalisation. Note that bias field correction should be performed prior to this step.

:ref:`dwipreproc` 
  Perform diffusion image pre-processing using FSL's eddy tool; including inhomogeneity distortion correction using FSL's topup tool if possible

:ref:`labelsgmfix` 
  In a FreeSurfer parcellation image, replace the sub-cortical grey matter structure delineations using FSL FIRST

:ref:`population_template` 
  Generates an unbiased group-average template from a series of images. First a template is optimised with linear registration (rigid or affine, affine is default), then non-linear registration is used to optimise the template further.

