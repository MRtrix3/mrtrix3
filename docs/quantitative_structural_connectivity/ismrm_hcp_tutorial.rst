ISMRM tutorial - Structural connectome for Human Connectome Project (HCP)
==========================================================================

.. WARNING:: This page describes an outdated example from 2015, currently retained only for historical reference. The steps described in this page are not up to date with current optimised practices. We do not recommend to use the steps below in connectomics processing for any project, using either HCP or other data.

This document duplicates the information provided during the *MRtrix3*
demonstration at ISMRM 2015 in Toronto. We will generate a structural
connectome for quintessential Human Connectome Project subject 100307.
Some of these instructions will be specific to HCP data, others will be
more general recommendations.

Note that this page is being retained as a reference of the steps
demonstrated during the ISMRM 2015 meeting; it does *not* constitute an
up-to-date 'recommended' processing pipeline for HCP data.

Necessary files
---------------

To duplicate our methods and results, you will need to download the
appropriate files, accessible through the following steps:

- https://db.humanconnectome.org/
- WU-Minn HCP Data - 900 Subjects + 7T
- Download Image Data: Single subject
- Session Type: 3T MRI
- Processing level: Preprocessed
- Package Type: MSM-Sulc + MSM-All
- add Structural Preprocessed and Diffusion Preprocessed to queue

The actual files within these compressed downloads that we will make use
of are:

Diffusion preprocessed files
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-  bvals
-  bvecs
-  data.nii.gz
-  nodif\_brain\_mask.nii.gz

Structural preprocessed files
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

-  aparc+aseg.nii.gz
-  T1w\_acpc\_dc\_restore_brain.nii.gz

Structural image processing
---------------------------

1. Generate a tissue-segmented image appropriate for
   `Anatomically-Constrained
   Tractography <anatomically-constrained-tractography-(ACT)>`__:

``5ttgen fsl T1w_acpc_dc_restore_brain.nii.gz 5TT.mif -premasked``

Note that it is *not necessary* to use a tissue-segmented image that has
the same resolution as the diffusion images; *MRtrix3* will happily acquire
interpolated values from each of them separately as tracking is
performed. This allows ACT to exploit the higher spatial resolution of
the tissue-segmented anatomical image, but still use the diffusion image
information at its native resolution also.

2. Collapse the multi-tissue image into a 3D greyscale image for
   visualisation:

``5tt2vis 5TT.mif vis.mif; mrview vis.mif``

If the tissue segmentation image contains clearly erroneous tissue
labels, you can delineate them manually using the ROI editor tool
in :ref:`mrview`, then apply your corrections to the tissue data using the
:ref:`5ttedit` command.

3. Modify the integer values in the parcellated image, such that the
   numbers in the image no longer correspond to entries in FreeSurfer's
   colour lookup table, but rows and columns of the connectome:

``labelconvert aparc+aseg.nii.gz FreeSurferColorLUT.txt fs_default.txt nodes.mif``

File ``FreeSurferColorLUT.txt`` is provided with FreeSurfer in its root
directory. The target lookup table file (``fs_default.txt`` in this case)
is a handy text file that provides a structure name for every row / column
of the connectome matrix: it is provided as part of *MRtrix3*, and located at
``shared/mrtrix3/labelconvert/fs_default.txt`` within the *MRtrix3* folder.

4. Replace FreeSurfer's estimates of sub-cortical grey matter structures
   with estimates from FSL's FIRST tool:

``labelsgmfix nodes.mif T1w_acpc_dc_restore_brain.nii.gz fs_default.txt nodes_fixSGM.mif -premasked``

Diffusion image processing
--------------------------

1. Convert the diffusion images into a non-compressed format (not
   strictly necessary, but will make subsequent processing faster),
   embed the diffusion gradient encoding information within the image
   header, re-arrange the data strides to make volume data contiguous
   in memory for each voxel, and convert to floating-point representation
   (makes data access faster in subsequent commands):

``mrconvert data.nii.gz DWI.mif -fslgrad bvecs bvals -datatype float32 -stride 0,0,0,1``

2. Generate a mean *b*\ =0 image (useful for visualisation):

``dwiextract DWI.mif - -bzero | mrmath - mean meanb0.mif -axis 3``

(If you are not familiar with the '\|' piping symbol, read more about it
`here <DesignPrinciples/Unix-Pipelines>`__)

3. `Estimate the response function <Response-function-estimation>`__;
   note that here we are estimating *multi-shell*, *multi-tissue*
   response functions:

``dwi2response msmt_5tt DWI.mif 5TT.mif RF_WM.txt RF_GM.txt RF_CSF.txt -voxels RF_voxels.mif``

``mrview meanb0.mif -overlay.load RF_voxels.mif -overlay.opacity 0.5`` (check
appropriateness of response function voxel selections)

4. Perform Multi-Shell, Multi-Tissue Constrained Spherical Deconvolution:

``dwi2fod msmt_csd DWI.mif RF_WM.txt WM_FODs.mif RF_GM.txt GM.mif RF_CSF.txt CSF.mif -mask nodif_brain_mask.nii.gz``

``mrconvert WM_FODs.mif - -coord 3 0 | mrcat CSF.mif GM.mif - tissueRGB.mif -axis 3``

This generates a 4D image with 3 volumes, corresponding to the tissue
densities of CSF, GM and WM, which will then be displayed in `mrview`
as an RGB image with CSF as red, GM as green and WM as blue (as was
presented in the MSMT CSD manuscript).

``mrview tissueRGB.mif -odf.load_sh WM_FODs.mif`` (visually make sure that
both the tissue segmentations and the white matter FODs are sensible)

Connectome generation
---------------------

1. Generate the initial tractogram:

``tckgen WM_FODs.mif 100M.tck -act 5TT.mif -backtrack -crop_at_gmwmi -seed_dynamic WM_FODs.mif -maxlength 250 -select 100M -cutoff 0.06``

Explicitly setting the maximum length is highly recommended for HCP
data, as the default heuristic - 100 times the voxel size - would result
in a maximum length of 125mm, which would preclude the reconstruction of
some longer pathways.

We also suggest a reduced FOD amplitude cutoff threshold for tracking when
using the MSMT CSD algorithm in conjunction with ACT; this allows streamlines
to reach the GM-WM interface more reliably, and does not result in
significant false positives since the MSMT algorithm does not produce many
erroneous small FOD lobes.

2. Apply the `Spherical-deconvolution Informed Filtering of Tractograms
   (SIFT) <sift>`__ algorithm

This method reduces the overall streamline count, but provides more
biologically meaningful estimates of structural connection density:

``tcksift 100M.tck WM_FODs.mif 10M_SIFT.tck -act 5TT.mif -term_number 10M``

If your system does not have adequate RAM to perform this process, the
first recommendation is to reduce the spatial resolution of the FOD
image and provide this alternative FOD image to SIFT (this should have
little influence on the outcome of the algorithm, but will greatly
reduce memory consumption):

``mrresize WM_FODs.mif FOD_downsampled.mif -scale 0.5 -interp sinc``

If this still does not adequately reduce RAM usage, you will need to
reduce the number of input streamlines to a level where your processing
hardware can successfully execute the :ref:`tcksift` command, e.g.:

``tckedit 100M.tck 50M.tck -number 50M``

Alternatively, if you're feeling brave, you can give
`SIFT2 <Handling-SIFT2-weights>`__ a try...

3. Map streamlines to the parcellated image to produce a connectome:

``tck2connectome 10M_SIFT.tck nodes_fixSGM.mif connectome.csv``

``mrview nodes_fixSGM.mif -connectome.init nodes_fixSGM.mif -connectome.load connectome.csv``

