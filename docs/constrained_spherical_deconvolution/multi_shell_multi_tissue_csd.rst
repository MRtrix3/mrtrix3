Multi-shell multi-tissue constrained spherical deconvolution
============================================================

Introduction
------------

Multi-Shell Multi-Tissue Constrained Spherical Deconvolution (MSMT-CSD)
exploits the unique b-value dependencies of the different macroscopic
tissue types (WM/GM/CSF) to estimate a multi-tissue orientation distribution
function (ODF) as explained in `Jeurissen et al. (2014) <#references>`__.
As it includes separate compartments for each tissue type, it can produce a map
of the WM/GM/CSF signal contributions directly from the DW data. In addition,
the more complete modelling of the DW signal results in more accurate apparent
fiber density (AFD) measures and more precise fibre orientation estimates
at the tissue interfaces.

User guide
----------

Multi-shell multi-tissue CSD can be performed as:

::

  dwi2fod msmt_csd dwi.mif wm_response.txt wmfod.mif gm_response.txt gm.mif csf_response.txt csf.mif

where

- ``dwi.mif`` is the dwi data set (input)

- ``<tissue>_response.txt`` is the tissue-specific response function (input)

- ``<tissue>.mif`` is the tissue-specific ODF (output), typically full FODs for WM and a single scalars for GM and CSF

Note that input response functions and their corresponding output ODFs need to be specified in pairs.

Typically, you will also want to use the ``-mask`` option to avoid unnecessary computations in non-brain voxels:

::

  dwi2fod msmt_csd -mask mask.mif dwi.mif wm_response.txt wmfod.mif gm_response.txt gm.mif csf_response.txt csf.mif

RGB tissue signal contribution maps can be obtained as follows:

::

  mrconvert -coord 3 0 wm.mif - | mrcat csf.mif gm.mif - vf.mif

The resulting WM FODs can be displayed together with the tissue signal contribution map as:

::

  mrview vf.mif -odf.load_sh wm.mif

Per tissue response function estimation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Input response functions for single-fibre WM, GM and CSF can be estimated directly from the data.
The most convenient way of doing so, is via the ``dwi2response dhollander`` algorithm
(`Dhollander et al. (2016) <#references>`__):

::

  dwi2response dhollander dwi.mif wm_response.txt gm_response.txt csf_response.txt
	
where

- ``dwi.mif`` is the same dwi data set as used above (input)

- ``<tissue>_response.txt`` is the tissue-specific response function as used above (output)

Note that the order of the tissue responses output for this algorithm is always: WM, GM, CSF.

Other methods exist, notably ``dwi2response msmt_5tt``, but this requires a co-registered T1 volume
and very accurate correction of EPI geometric distortions (both up to sub-voxel accuracy), as well as
accurate segmentation of the T1 volume.
Even then, still, ``dwi2response msmt_5tt`` may be less accurate than ``dwi2response dhollander``
in a range of scenarios (`Dhollander et al. (2016) <#references>`__).

References
----------

1. B. Jeurissen, J.D. Tournier, T. Dhollander, A. Connelly, and J.
   Sijbers. *Multi-tissue constrained spherical deconvolution for
   improved analysis of multi-shell diffusion MRI data.* NeuroImage, 103
   (2014), pp. 411–426 [`SD
   link <http://www.sciencedirect.com/science/article/pii/S1053811914006442>`__\ ]

2. T. Dhollander, D. Raffelt, and A. Connelly. *Unsupervised 3-tissue response
   function estimation from single-shell or multi-shell diffusion MR data without
   a co-registered T1 image.* ISMRM Workshop on Breaking the Barriers of Diffusion MRI (2016), pp. 5 [`full text
   link <https://www.researchgate.net/publication/307863133_Unsupervised_3-tissue_response_function_estimation_from_single-shell_or_multi-shell_diffusion_MR_data_without_a_co-registered_T1_image>`__\ ]

