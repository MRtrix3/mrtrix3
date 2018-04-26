Global tractography
===================

Introduction
------------

Global tractography is the process of finding the full track
configuration that best explains the measured DWI data. As opposed to
streamline tracking, global tractography is less sensitive to noise, and
the density of the resulting tractogram is directly related to the data
at hand.

As of version 3.0, MRtrix supports global tractography using a
multi-tissue spherical convolution model, as introduced in `Christiaens
et al. (2015) <#references>`__. This method extends the method of
`Reisert et al. (2011) <#references>`__ to multi-shell response
functions, estimated from the data, and adopts the multi-tissue model
presented in `Jeurissen et al. (2014) <#references>`__ to account for
partial voluming.

User guide
----------

For multi-shell DWI data, the most common use will be:

::

  tckglobal dwi.mif wm_response.txt -riso csf_response.txt -riso gm_response.txt -mask mask.mif -niter 1e9 -fod fod.mif -fiso fiso.mif tracks.tck

In this example, ``dwi.mif`` is the input dataset, including the
gradient table, and ``tracks.tck`` is the output tractogram. ``wm_response.txt``, 
``gm_response.txt`` and ``csf_response.txt`` are tissue response functions (cf. next 
section). Optional output images fod.mif and fiso.mif contain the 
predicted WM fODF and isotropic tissue fractions of CSF and GM 
respectively, estimated as part of the global optimization and thus 
affected by spatial regularization. 

Per tissue response function estimation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Input response functions for single-fibre WM, GM and CSF can be estimated directly from the data.
An up to date overview of our methods for responce function estimation can be found on the
`response function estimation <response_function_estimation>`__ page.

In the original global tractography paper (`Christiaens et al. (2015) <#references>`__), 
response functions were estimated using a prior tissue segmentation obtained from a 
coregistered structural T1 scan. For the WM response, a further FA threshold was used.
This pipeline can be most closely replicated using the 
``5ttgen`` and ``dwi2response msmt_5tt`` commands in this fashion: 

::

  5ttgen fsl T1.mif 5tt.mif
  dwi2response msmt_5tt dwi.mif 5tt.mif wm_response.txt gm_response.txt csf_response.txt -wm_algo fa

where

- ``T1.mif`` is a coregistered T1 data set from the same subject (input)

- ``5tt.mif`` is the resulting tissue type segmentation, used subsequently used in the response function estimation (output/input)

- ``dwi.mif`` is the same dwi data set as used above (input)

- ``<tissue>_response.txt`` is the tissue-specific response function as used above (output)

The difference between these instructions and the method described in `Christiaens et al. (2015) <#references>`__ is that instead of selecting WM single-fibre voxels using an absolute FA threshold of 0.75, the 300 voxels with the highest FA value inside the WM segmentation are used.

Note that this process is dependent on accurate correction of EPI geometric distortions, and rigid-body registration between the DWI and T1 modalities, such that the T1 image can be reliably used to select pure-tissue voxels in the DWI volumes. Failure to achieve these may result in inappropriate voxels being used for response function estimation, with concomitant errors in tissue estimates.

Parameters
~~~~~~~~~~

``-niter``: The number of iterations in the optimization. Although the
default value is deliberately kept low, a full brain reconstruction will
require at least 100 million iterations.

``-lmax``: Maximal order of the spherical harmonics basis.

``-length``: Length of each track segment (particle), which determines
the resolution of the reconstruction.

``-weight``: Weight of each particle. Decreasing its value by a factor
of two will roughly double the number of reconstructed tracks, albeit at
increased computation time.

Particle potential ``-ppot``: The particle potential essentially
associates a *cost* to each particle, relative to its weight. As such,
we are in fact trying to reconstruct the data as well as possible, with
as few particles as needed. This ensures that there is sufficient
*proof* for each individual particle, and hence avoids that a bit of
noise in the data spurs generation of new (random) particles. Think of
it as a parameter that balances sensitivity versus specificity. A higher
particle potential requires more *proof* in the data and therefore leads
to higher specificity; a smaller value increases sensitivity.

Connection potential ``-cpot``: The connection potential is the driving
force for connecting segments and hence building tracks. Higher values
increase connectivity, at the cost of increased invalid connections.

Ancillary outputs
~~~~~~~~~~~~~~~~~

``-fod``: Outputs the predicted fibre orientation distribution function 
(fODF) as an image of spherical harmonics coefficients. 
This fODF is estimated as part of the global track optimization, and
therefore incorporates the spatial regularization that it imposes.
Internally, the fODF is represented as a discrete sum of apodized point
spread functions (aPSF) oriented along the directions of all particles in
the voxel, akin to track orientation distribution imaging (TODI, 
`Dhollander et al., 2014 <#references>`__). This internal representation 
is used to predict the DWI signal upon every change to the particle 
configuration.

``-fiso``: Outputs the estimated density of all isotropic tissue
components, as multiple volumes in one 4-D image in the same order as
their respective ``-riso`` kernels were provided.

``-eext``: Outputs the residual data energy image, including the
L1-penalty imposed by the particle potential.

References
----------

1. D. Christiaens, M. Reisert, T. Dhollander, S. Sunaert, P. Suetens,
   and F. Maes. *Global tractography of multi-shell diffusion-weighted
   imaging data using a multi-tissue model.* NeuroImage, 123 (2015) pp.
   89–101 [`SD
   link <http://www.sciencedirect.com/science/article/pii/S1053811915007168>`__\ ]

2. M. Reisert, I. Mader, C. Anastasopoulos, M. Weigel, S. Schnell, and
   V. Kiselev. *Global fiber reconstruction becomes practical.*
   NeuroImage, 54 (2011) pp. 955–962 [`SD
   link <http://www.sciencedirect.com/science/article/pii/S1053811910011973>`__\ ]

3. B. Jeurissen, J.D. Tournier, T. Dhollander, A. Connelly, and J.
   Sijbers. *Multi-tissue constrained spherical deconvolution for
   improved analysis of multi-shell diffusion MRI data.* NeuroImage, 103
   (2014), pp. 411–426 [`SD
   link <http://www.sciencedirect.com/science/article/pii/S1053811914006442>`__\ ]

4. T. Dhollander, L. Emsell, W. Van Hecke, F. Maes, S. Sunaert, and P.
   Suetens. *Track Orientation Density Imaging (TODI) and Track
   Orientation Distribution (TOD) based tractography.* NeuroImage, 94
   (2014), pp. 312–336 [`SD
   link <http://www.sciencedirect.com/science/article/pii/S1053811913012676>`__\ ]

