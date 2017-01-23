DWI denoising
^^^^^^^^^^^^^

The effective SNR of diffusion data can be improved considerably by exploiting the redundancy in the data to reduce the effects of thermal noise. This functionality is provided in the command ``dwidenoise``::

    foreach * : dwidenoise IN/dwi.mif IN/dwi_denoised.mif

Note that this denoising step *must* be performed prior to any other image pre-processing: any form of image interpolation (e.g. re-gridding images following motion correction) will invalidate the statistical properties of the image data that are exploited by :ref:`dwidenoise`, and make the denoising process prone to errors. Therefore this process is applied as the very first step.
