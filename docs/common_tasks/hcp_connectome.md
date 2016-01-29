# Structural connectome for Human Connectome Project (HCP)

This document duplicates the information provided during the MRtrix demonstration at ISMRM 2015 in Toronto. We will generate a structural connectome for quintessential Human Connectome Project subject 100307. Some of these instructions will be specific to HCP data, others will be more general recommendations.

## Necessary files

To duplicate our methods and results, you will need to download the appropriate files:
- https://db.humanconnectome.org/
- WU-Minn HCP Data – 500 Subjects + MEG2
- Download images - Single subject
- Processing level: Preprocessed
- Structural Preprocessed and Diffusion Preprocessed

The actual files within these compressed downloads that we will make use of are:

### Diffusion preprocessed files

* bvals
* bvecs
* data.nii.gz
* nodif_brain_mask.nii.gz

### Structural preprocessed files

* aparc+aseg.nii.gz
* T1w_acpc_dc_restore.nii.gz

## Diffusion image processing

1. Convert the diffusion images into a non-compressed format (not strictly necessary, but will make subsequent processing faster), embed the diffusion gradient encoding information within the image header, extract only those volumes corresponding to _b_=0 and the largest _b_-value shell, re-arrange the data strides to make volume data contiguous in memory for each voxel and convert to floating-point representation (makes data access faster in subsequent commands):

   `dwiextract data.nii.gz - -fslgrad bvecs bvals -shell 0,3000 | mrconvert - DWI.mif -datatype float32 -stride 0,0,0,1`
   
   (If you are not familiar with the '|' piping symbol, read more about it [here](DesignPrinciples/Unix-Pipelines))
   
2. Generate a mean _b_=0 image (useful for visualisation):

   `dwiextract DWI.mif - -bzero | mrmath - mean meanb0.mif -axis 3`
   
3. [Estimate the response function](Response-function-estimation); note that we erode the brain mask first, as this prevents voxels near the edge of the brain from potentially being erroneously selected as single-fibre voxels:

   `maskfilter nodif_brain_mask.nii.gz erode - -npass 6 | dwi2response DWI.mif response.txt -mask - -lmax 8 -sf sf_mask.mif`
   
   `mrview meanb0.mif -roi.load sf_mask.mif -roi.opacity 0.5` (check appropriateness of single-fibre voxel selection)
   
   `shview response.txt` (visualise response function profile)
   
   _Coming soon: Fully-automated multi-tissue response function estimation for multi-shell data_
   
4. Perform Constrained Spherical Deconvolution:

   `dwi2fod DWI.mif response.txt FOD.mif -mask nodif_brain_mask.nii.gz`
   
   `mrview meanb0.mif -odf.load FOD.mif` (visually make sure that the FODs are sensible)
   
   _Coming soon: Multi-shell, multi-tissue spherical deconvolution_
   
   _Coming soon: Robust Constrained Spherical Deconvolution - Rician bias correction, outlier rejection & spatial regularisation_
   
## Structural image processing

1. Generate a tissue-segmented image appropriate for [Anatomically-Constrained Tractography](anatomically-constrained-tractography-(ACT)):

   `act_anat_prepare_fsl T1w_acpc_dc_restore.nii.gz 5TT.mif`
   
   Note that it is _not necessary_ to use a tissue-segmented image that has the same resolution as the diffusion images; MRtrix will happily acquire interpolated values from each of them separately as tracking is performed.
   
2. Collapse the multi-tissue image into a 3D greyscale image for visualisation:

   `5tt2vis 5TT.mif vis.mif; mrview vis.mif`
   
   If the tissue segmentation image contains clearly erroneous tissue labels, you can delineate them manually using `mrview`'s ROI editor tool, then apply your corrections to the tissue data using the `5ttedit` command.

3. Modify the integer values in the parcellated image, such that the numbers in the image no longer correspond to entries in FreeSurfer's colour lookup table, but rows and columns of the connectome:

   `labelconfig aparc+aseg.nii.gz fs_default.txt nodes.mif -lut_freesurfer FreeSurferColorLUT.txt`
   
   The configuration file (`fs_default.txt` in this case) is also a handy text file that provides a structure name for every row / column of the connectome matrix. It is provided as part of MRtrix3, and located at `src/connectome/config/fs_default.txt` within the MRtrix3 folder.

4. Replace FreeSurfer's estimates of sub-cortical grey matter structures with estimates from FSL's FIRST tool:

   `fs_parc_replace_sgm_first nodes.mif T1w_acpc_dc_restore.nii.gz fs_default.txt nodes_fixSGM.mif`

## Connectome generation

1. Generate the initial tractogram:

   `tckgen FOD.mif 100M.tck -act 5TT.mif -backtrack -crop_at_gmwmi -seed_dynamic FOD.mif -maxlength 250 -number 100000000`
   
   Explicitly setting the maximum length is highly recommended for HCP data, as the default heuristic - 100 times the voxel size - would result in a maximum length of 125mm, which would preclude the reconstruction of some longer pathways.
   
2. Apply the [Spherical-deconvolution Informed Filtering of Tractograms (SIFT)](sift) method, which reduces the overall streamline count, but provides more biologically meaningful estimates of structural connection density:

   `tcksift 100M.tck FOD.mif 10M_SIFT.tck -act 5TT.mif -term_number 10000000`
   
   If your system does not have adequate RAM to perform this process, the first recommendation is to reduce the spatial resolution of the FOD image and provide this alternative FOD image to SIFT (this should have little influence on the outcome of the algorithm, but will greatly reduce memory consumption):
   
   `mrresize FOD.mif FOD_downsampled.mif -scale 0.5 -interp sinc`
   
   If this still does not adequately reduce RAM usage, you will need to reduce the number of input streamlines to a level where your processing hardware can successfully execute the `tcksift` command, e.g.:
   
   `tckedit 100M.tck 50M.tck -number 50000000`
   
   _Coming soon: SIFT2 - biologically accurate connection densities without discarding streamlines_

3. Map streamlines to the parcellated image to produce a connectome:

   `tck2connectome 10M_SIFT.tck nodes_fixSGM.mif connectome.csv`
   
   _Coming soon: Connectome visualisation in MRtrix_


