If denoising and/or Gibbs ringing removal are performed as part of the preprocessing, they *must* be performed *prior* to any other processing steps: most other processing steps, in particular those that involve interpolation of the data, will invalidate the original properties of the image data that are exploited by :ref:`dwidenoise` and :ref:`mrdegibbs` at this stage, and would render the result prone to errors.

If denoising is included, it's performed as the first step:

    foreach * : dwidenoise IN/dwi.mif IN/dwi_denoised.mif

If Gibbs ringing removal is included, it follows immediately after:

    foreach * : mrdegibbs IN/dwi_denoised.mif IN/dwi_denoised_unringed.mif -axes 0,1
    
.. WARNING:: The :code:`-axes` option to :ref:`mrdegibbs` is used to specify in which plane the slices were acquired. The :code:`-axes 0,1` in the example above refers to the x-y plane, which is appropriate for data consisting of a stack of *axial* slices (assuming a typical human scanner and subject). For typical human data, change this to :code:`-axes 0,2` for *coronal* slices or :code:`-axes 1,2` for *sagittal* slices.

