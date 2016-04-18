Multi-tissue constrained spherical deconvolution
================================================

Introduction
------------

Multi-tissue constrained spherical deconvolution (CSD) of multi-shell data exploits the unique b-value dependencies of the different macroscopic tissue types (WM/GM/CSF) to estimate a multi-tissue orientation distribution function (ODF) as explained in `Jeurissen et al. (2014) <#references>`__. As it includes separate compartments for each tissue type, it can produce a map of the WM/GM/CSF volume fractions directly from the DW data. In addition, the more complete modelling of the DW signal results in more accurate apparent fiber density (AFD) measures and more precise fibre orientation estimates at the tissue interfaces.

User guide
----------

Multi-tissue CSD can be performed as:

::

  msdwi2fod dwi.mif csf.txt csf.mif gm.txt gm.mif wm.txt wm.mif

where

- ``dwi.mif`` is the dwi data set (input)

- ``<tissue>.txt`` is the tissue-specific response function (input)

- ``<tissue>.mif`` is the tissue-specific ODF (output)

Note that input response functions and their corresponding output ODFs need to be specified in pairs.

Typically, you will also want to use the ``-mask`` to avoid calculations in non-brain voxels:

::

  msdwi2fod -mask mask.mif dwi.mif csf.txt csf.mif gm.txt gm.mif wm.txt wm.mif

RGB tissue volume fraction maps can be obtained as follows:

::

  mrconvert -coord 3 0 wm.mif - | mrcat csf.mif gm.mif - vf.mif

The resulting WM fODFs can be displayed together with the tissue volume fraction map as:

::

  mrview vf.mif -odf.load_sh wm.mif

Per tissue response function estimation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Input response functions for CSF, GM and single fibre WM can be estimated from the data using prior tissue segmentations, similarly to that described in `Jeurissen et al. (2014) <#references>`__ using the ``dwi2response msmt_5tt`` command: 

::

  dwi2response msmt_5tt dwi.mif 5tt.mif gm.txt wm.txt csf.txt
	
where

- ``dwi.mif`` is the same dwi data set as used above (input)

- ``5tt.mif`` is a tissue type segmentation of a coregistered T1 data set from the same subject (input)

- ``<tissue>.txt`` is the tissue-specific response function as used above (output)

Prior tissue type segmentation can be obtained from a structural T1 scan using the `5ttgen` script:

::

  5ttgen fsl T1.mif 5tt.mif

where

- ``T1.mif`` is a coregistered T1 data set from the same subject (input)

- ``5tt.mif`` is the tissue type segmentation used above (output)

The difference between the default behaviour of ``dwi2response msmt_5tt`` and the method described in `Jeurissen et al. (2014) <#references>`__ is that instead of selecting WM single-fibre voxels using an FA threshold, the ``dwi2response tournier`` algorithm is instead used.

Note that this process is dependent on accurate correction of EPI geometric distortions, and rigid-body registration between the DWI and T1 modalities, such that the T1 image can be reliably used to select pure-tissue voxels in the DWI volumes. Failure to achieve these may result in inappropriate voxels being used for response function estimation, with concomitant errors in tissue estimates.

References
----------

1. B. Jeurissen, J.D. Tournier, T. Dhollander, A. Connelly, and J.
   Sijbers. *Multi-tissue constrained spherical deconvolution for
   improved analysis of multi-shell diffusion MRI data.* NeuroImage, 103
   (2014), pp. 411–426 [`SD
   link <http://www.sciencedirect.com/science/article/pii/S1053811914006442>`__\ ]
