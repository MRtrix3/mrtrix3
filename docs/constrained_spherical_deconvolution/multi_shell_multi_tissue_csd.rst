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

Input response functions for single-fibre WM, GM and CSF can be estimated from 
the data using any of the available multi-tissue `response function estimation 
<response_function_estimation>`__ methods.

In the original MT-CSD paper (`Jeurissen et al. (2014) <#references>`__), response
functions were estimated using a prior tissue segmentation obtained from a coregistered 
structural T1 scan. This pipeline can be most closely replicated using the ``5ttgen``
and ``dwi2response msmt_5tt`` commands: 

::

  5ttgen fsl T1.mif 5tt.mif
  dwi2response msmt_5tt dwi.mif 5tt.mif wm_response.txt gm_response.txt csf_response.txt
	
where

- ``T1.mif`` is a coregistered T1 data set from the same subject (input)

- ``5tt.mif`` is the resulting tissue type segmentation, used subsequently used in the response function estimation (output/input)

- ``dwi.mif`` is the same dwi data set as used above (input)

- ``<tissue>_response.txt`` is the tissue-specific response function as used above (output)

The difference between the default behaviour of ``dwi2response msmt_5tt`` and the method described in `Jeurissen et al. (2014) <#references>`__ is that instead of selecting WM single-fibre voxels using an FA threshold, the ``dwi2response tournier`` algorithm is used.

Note that this process is dependent on accurate correction of EPI geometric distortions, and rigid-body registration between the DWI and T1 modalities, such that the T1 image can be reliably used to select pure-tissue voxels in the DWI volumes. Failure to achieve these may result in inappropriate voxels being used for response function estimation, with concomitant errors in tissue estimates.


References
----------

1. B. Jeurissen, J.D. Tournier, T. Dhollander, A. Connelly, and J.
   Sijbers. *Multi-tissue constrained spherical deconvolution for
   improved analysis of multi-shell diffusion MRI data.* NeuroImage, 103
   (2014), pp. 411–426 [`SD
   link <http://www.sciencedirect.com/science/article/pii/S1053811914006442>`__\ ]


