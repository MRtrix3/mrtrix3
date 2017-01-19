Fibre Density and Cross-section - Single Shell DWI
==================================================

Introduction
-------------

This tutorial explains how to perform `fixel-based analysis of fibre density and cross-section <https://www.ncbi.nlm.nih.gov/pubmed/27639350>`_ using single-shell data. While the focus here is on the analysis of `Apparent Fibre Density (AFD) <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_ derived from FODs, other fixel-based measures related to fibre density can also be analysed with a few minor modifications to these steps (as outlined below). We note that high b-value (>2000s/mm2) data is recommended to aid the interpretation of AFD being related to the intra-axonal space. See the `original paper <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_ for more details.

Note that for all MRtrix scripts and commands, additional information on the command usage and available command-line options can be found by invoking the command with the :code:`-help` option. Please post any questions or issues on the `MRtrix community forum <http://community.mrtrix.org/>`_.


Pre-processsing steps
---------------------
Below is a list of recommended pre-processing steps for fixel-based analysis, with some specific steps for quantitative analysis of AFD.


1. DWI denoising
^^^^^^^^^^^^^^^^

The effective SNR of diffusion data can be improved considerably by exploiting the redundancy in the data to reduce the effects of thermal noise. This functionality is provided in the command ``dwidenoise``::

  dwidenoise <input_dwi> <output_dwi>

Note that this denoising step *must* be performed prior to any other image pre-processing: any form of image interpolation (e.g. re-gridding images following motion correction) will invalidate the statistical properties of the image data that are exploited by :ref:`dwidenoise`, and make the denoising process prone to errors. Therefore this process is applied as the very first step.


2. DWI general pre-processing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :code:`dwipreproc` script is provided for performing general pre-processing of diffusion image data - this includes eddy current-induced distortion correction, motion correction, and (possibly) susceptibility-induced distortion correction. Commands for performing this pre-processing are not yet implemented in *MRtrix3*; the :code:`dwipreproc` script in its current form is in fact an interface to the relevant commands that are provided as part of the `FSL <http://fsl.fmrib.ox.ac.uk/>`_ package. Installation of FSL (including `eddy <http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/EDDY>`_) is therefore required to use this script, and citation of the relevant articles is also required (see the :ref:`dwipreproc` help page).

Usage of this script varies depending on the specific nature of the DWI acquisition with respect to EPI phase encoding - full details are available within the :ref:`DWI distortion correction using ``dwipreproc`` ` page, and the :ref:`dwipreproc` help file.

Here, only a simple example is provided, where a single DWI series is acquired where all volumes have an anterior-posterior (A>>P) phase encoding direction::

  dwipreproc <input_dwi> <output_dwi> -rpe_none -pe_dir AP


3. Estimate a brain mask
^^^^^^^^^^^^^^^^^^^^^^^^^
A whole-brain mask is required as input to subsequent steps. This can be computed with::

  dwi2mask <input_dwi> <output_mask>



AFD-specific pre-processsing steps
--------------------------------

To enable robust quantitative comparisons of AFD across subjects three additional steps are required. Note these can be skipped if analysing other DWI fixel-based measures related to fibre density (for example CHARMED).


4. Bias field correction
^^^^^^^^^^^^^^^^^^^^^^^^
Because we recommend a :ref:`global intensity normalisation <global-intensity-normalisation>`, bias field correction is required as a pre-processing step to eliminate low frequency intensity inhomogeneities across the image. DWI bias field correction is perfomed by first estimating a correction field from the DWI b=0 image, then applying the field to correct all DW volumes. This can be done in a single step using the :ref:`dwibiascorrect` script in MRtrix. The script uses bias field correction algorthims available in `ANTS <http://stnava.github.io/ANTs/>`_ or `FSL <http://fsl.fmrib.ox.ac.uk/>`_. In our experience the `N4 algorithm <http://www.ncbi.nlm.nih.gov/pmc/articles/PMC3071855/>`_ in ANTS gives superiour results. To install N4 install the `ANTS <http://stnava.github.io/ANTs/>`_ package, then run perform bias field correction on DW images using::

    dwibiascorrect -ants -mask <input_brain_mask> <input_dwi> <output_corrected_dwi>


5. Global intensity normalisation across subjects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As outlined :ref:`here <global-intensity-normalisation>`, a global intensity normalisation is recommended for AFD analysis. For single-shell data this can be achieved using the :ref:`dwiintensitynorm` script. The script performs normalisation on all subject within a study, and therefore the input and output arguments are folders containing all study images:

    dwiintensitynorm <input_dwi_folder> <input_brain_mask_folder> <output_normalised_dwi_folder> <output_fa_template> <output_template_wm_mask>

The dwiintensitynorm script also outputs the study-specific FA template and white matter mask. **It is recommended that you check that the white matter mask is appropriate** (i.e. does not contain CSF or voxels external to the brain. Note it only needs to be a rough WM mask). If you feel the white matter mask needs to be larger or smaller you can re-run :code:`dwiintensitynorm` with a different :code:`-fa_threshold` option. Note that if your input brain masks include CSF then this can cause spurious high FA values outside the brain which will may be included in the template white matter mask.

Keeping the FA template image and white matter mask is also handy if additional subjects are added to the study at a later date. New subjects can be intensity normalised in a single step by `piping <http://userdocs.mrtrix.org/en/latest/getting_started/command_line.html#unix-pipelines>`_ the following commands together::

    dwi2tensor <input_dwi> -mask <input_brain_mask> - | tensor2metric - -fa - | mrregister <fa_template> - -mask2 <input_brain_mask> -nl_scale 0.5,0.75,1.0 -nl_niter 5,5,15 -nl_warp - tmp.mif | mrtransform <input_template_wm_mask> -template <input_dwi> -warp - - | dwinormalise <input_dwi> - <output_normalised_dwi>; rm tmp.mif

.. NOTE:: The above command may also be useful if you wish to alter the mask then re-apply the intensity normalisation to all subjects in the study. For example you may wish to edit the mask using the ROI tool in :code:`mrview` to remove white matter regions that you hypothesise are affected by the disease (e.g. removing the corticospinal tract in a study of motor neurone disease due to T2 hyperintensity). You also may wish to redefine the mask completely, for example in an elderly population (with larger ventricles) it may be appropriate to intensity normalise using the median b=0 CSF. This could be performed by manually masking partial-volume-free CSF voxels, then running the above command with the CSF mask instead of the <input_template_wm_mask>.

.. WARNING:: We also strongly recommend you that you check the scale factors applied during intensity normalisation are not influenced by the variable of interest in your study. For example if one group contains global changes in white matter T2 then this may directly influence the intensity normalisation and therefore bias downstream results. To check this we recommend you perform an equivalence test to ensure mean scale factors are the same between groups. To output the scale factor applied for each subject use :code:`mrinfo <output_normalised_dwi> -property dwi_norm_scale_factor`.

6. Computing a group average response function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
As described `here <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_, using the same response function when estimating FOD images for all subjects enables differences in the intra-axonal volume (and therefore DW signal) across subjects to be detected as differences in the FOD amplitude (the AFD). At high b-values (~3000 s/mm2), the shape of the estimated white matter response function varies little across subjects and therefore choosing any single subjects' estimate response is OK. To estimate a response function from a single subject::

    dwi2response tournier <Input DWI> <Output response text file>

Alternatively, to ensure the response function is representative of your study population, a group average response function can be computed by first estimating a response function per subject, then averaging with the script::

    average_response <input_response_files (mulitple inputs accepted)> <output_group_average_response>



Fixel-based analysis steps
---------------------------

7. Upsampling DW images
^^^^^^^^^^^^^^^^^^^^^^^
Upsampling DWI data before computing FODs can `increase anatomical contrast <http://www.sciencedirect.com/science/article/pii/S1053811914007472>`_ and improve downstream spatial normalisation and statistics. We recommend upsampling by a factor of two using bspline interpolation::

    mrresize <input_dwi> -scale 2.0 <output_upsampled_dwi>
    
8. Compute upsampled brain mask images
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Compute a whole brain mask from the upsampled DW images::
    
    dwi2mask <input_upsampled_dwi> <output_upsampled_mask>

Depending on your data, you may find that upsampling the low-resolution masks from step 3 gives superiour masks (with less holes). This can be performed using::

    mrresize <input_mask> -scale 2.0 -inter nearest <output_upsampled_mask>

9. Fibre Orientation Distribution estimation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
This command performs Constrained Spherical Deconvolution (CSD) using the group average response function `estimated previously  <http://userdocs.mrtrix.org/en/latest/workflows/DWI_preprocessing_for_quantitative_analysis.html>`_. Note that :code:`dwi2fod csd` can be used, however here we use :code:`dwi2fod msmt_csd` (even with single shell data) to benefit from the hard non-negativity constraint::

    dwiextract <input_upsampled_dwi> - | dwi2fod msmt_csd - <group_average_response_text_file> <output_fod_image> -mask <input_upsampled_mask>

10. Generate a study-specific unbiased FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Population template creation is the most time consuming step in a fixel-based analysis. If you have a large number of subjects in your study, we recommend building the template from a subset of 20-40 individuals. Subjects should be chosen to ensure the generated template is representative of your population (i.e. equal number of patients and controls). To build a template, place all FOD images in a single folder. We also recommend placing a set of corresponding mask images (with the same prefix as the FOD images) in another folder. Using masks can speed up registration significantly. Run the population_template building script as follows::
    
    population_template <input_folder_of_FOD_images> -mask_dir <input_mask_folder> <output_fod_template_image>

.. NOTE::If you are building a template from your entire study population use the -warp_dir option to output a folder containing all subject warps to the template. Saving the warps here will enable you to skip the next step. 

11. Register all subject FOD images to the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Register the FOD image from all subjects to the FOD template image::

    mrregister <input_fod_image> -mask1 <input_subject_mask> <input_fod_template_image> -nl_warp <subject2template_warp> <template2subject_warp>


12. Compute the intersection of all subject masks in template space
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Different subjects will have subtly different brain coverage. To ensure subsequent analysis is performed in voxels that contain data from all subjects, we warp all subject masks into template space and compute the mask intersection. For each subject::
    
    mrtransform <input_upsampled_mask_image> -warp <subject2template_warp> -interp nearest <output_warped_mask>

Compute the intersection of all warped masks::
    
    mrmath <input_all_warped_masks_multiple_inputs> min <output_template_mask_intersection>
    
    
13. Compute a white matter template analysis fixel mask
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Here we perform a 2-step threshold to identify template white matter fixels to be included in the analysis. Fixels in the template fixel analysis mask are also used to identify the best fixel correspondence across all subjects (i.e. match fixels across subjects within a voxel).
       
Compute a template AFD peaks fixel image::
    
    fod2fixel fod_template.mif -mask template_mask_intersection.mif fixel_directory<output -peak peaks.mif
    
.. NOTE:: Fixel images in this step are stored using the :ref:`fixel_format`.
    
Next view the peaks file using the fixel plot tool in :ref:`mrview` and identify an appropriate threshold that removes peaks from grey matter, yet does not introduce any 'holes' in your white matter (approximately 0.33).

Threshold the peaks fixel image::
    
    mrthreshold <input_fixel_directory_step1/peaks.mif> 0.33 <fixel_directory/mask.mif>

Generate an analysis voxel mask from the fixel mask. The median filter in this step should remove spurious voxels outside the brain, and fill in the holes in deep white matter where you have small peaks due to 3-fibre crossings::

    fixel2voxel <fixel_directory/mask.mif> count - | mrthreshold - - -abs 0.5 | mrfilter - median <output_analysis_voxel_mask>

Recompute the fixel mask using the analysis voxel mask. Using the mask allows us to use a lower AFD threshold than possible in the steps above, to ensure we have included fixels with low AFD inside white matter::
 
    fod2fixel -mask <input_analysis_voxel_mask> <input_fod_template_image> <output_fixel_directory> -peak peaks.mif
    fixelthreshold <input_temp.msf> -crop 0.2 <output_analysis_fixel_mask.msf> -force
    rm <temp.msf>
    
.. NOTE:: We recommend having no more than 500,000 fixels in the analysis_fixel_mask (you can check this with :code:`fixelstats`), otherwise downstream statistical analysis (using :ref:`fixelcfestats`) will run out of RAM). A mask with 500,000 fixels will require a PC with 128GB of RAM for the statistical analysis step.

14. Warp FOD images to template space
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Note that here we warp FOD images into template space *without* FOD reorientation. Reorientation will be performed in a separate subsequent step::

    mrtransform <input_subject_fod_image> -warp <subject2template_warp> -noreorientation <output_warped_fod_image>

15. Segment FOD images to estimate fixels and their fibre density (FD)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Here we segment each FOD lobe to identify the number and orientation of fixels in each voxel. The output also contains the apparent fibre density (AFD) value per fixel estimated as the FOD lobe integral (see `here <http://www.sciencedirect.com/science/article/pii/S1053811912011615>`_ for details on FOD segmentation). Note that in the following steps we will use a more generic shortened acronym - Fibre Density (FD) instead of AFD for consistency with our recent work (paper under review)::

    fod2fixel <input_warped_fod_image> -mask <input_analysis_voxel_mask> <output_fixel_folder> -afd <fd.mif>
    
.. NOTE:: If you would like to perform fixel-based analysis of metrics derived from other diffusion MRI models (e.g. CHARMED), replace steps 14 & 15. For example, in step 14 you can warp preprocessed DW images (also without any reorientation). In step 15 you could then estimate your DWI model of choice, and output the FD related measure to the :ref:`fixel_format`, ready for the subsequent fixel reorientation step.
    
    
16. Reorient fixel orientations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Here we reorient the direction of all fixels based on the Jacobian matrix (local affine transformation) at each voxel in the warp. Note that in-place fixel reorientation can be performed by specifing the output fixel folder to be the same as the input, and using the :code:`-force` option::

    fixelreorient <input_fixel_folder> <subject2template_warp> <output_fixel_folder>
    
17. Assign subject fixels to template fixels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In step 8 we obtained spatial correspondence between subject and template. In step 10 we corrected the fixel orientations to ensure angular correspondence of the segmented peaks of subject and template. Here, for each fixel in the template fixel analysis mask, we identify the corresponding fixel in each voxel of the subject image and assign the FD value of the subject fixel to the corresponding fixel in template space. If no fixel exists in the subject that corresponds to the template fixel then it is assigned a value of zero. See `this paper <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_ for more information. In the command below, we recommend the :code:`output_fixel_folder` is the same folder for all subjects (called something like "all_subject_data". This folder can be directly input to the :code:`fixelcfestats` command in step 16 below::

    fixelcorrespondence <input_fixel_folder/fd.mif> <template_fixel_folder> <output_fixel_folder> <subj01_fd.mif> -force
    
18. Compute fibre cross-section (FC) metric
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Apparent fibre density, and other related measures that are influenced by the quantity of restricted water, only permit the investigation of group differences in the number of axons that manifest as a change to *within-voxel* density. However, depending on the disease type and stage, changes to the number of axons may also manifest as macroscopic differences in brain morphology. This step computes a fixel-based metric related to morphological differences in fibre cross-section, where information is derived entirely from the warps generated during registration (paper in press). In the command below, we recommend the :code:`output_fixel_folder` is the same folder for all subjects (called something like "all_subject_data"). This folder can be the same as the output folder used in the previous step.::

    warp2metric <subject2template_warp> -fc <template_fixel_folder> <output_fixel_folder <subj01_fc.mif>
    
Note that the FC files will be used in the next step. However, for group statistical analysis of FC we recommend taking the log (FC) to ensure data are centred about zero and normally distributed::

    mrcalc <subj01_fc.mif> -log <subj01_log_fc.mif>

19. Compute a combined measure of fibre density and cross-section (FDC)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To account for changes to both within-voxel fibre density and macroscopic atrophy, fibre density and fibre cross-section must be combined (a measure we call fibre density & cross-section, FDC). This enables a more complete picture of group differences in white matter. Note that as discussed in our future work (under review), group differences in FD or FC alone must be interpreted with care in crossing-fibre regions. However group differences in FDC are more directly interpretable. To generate the combined measure we 'modulate' the FD by FC::

    mrcalc <subj01_fd.mif> <subj01_fc.mif> -mult <subj01_fdc.mif>
    
20. Perform whole-brain fibre tractography on the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Statistical analysis using `connectivity-based fixel enhancement <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_ exploits connectivity information derived from probabilistic fibre tractography. To generate a whole-brain tractogram from the FOD template::
    
    tckgen -angle 22.5 -maxlen 250 -minlen 10 -power 1.0 <input_fod_template_image> -seed_image <input_analysis_voxel_mask> -mask <input_analysis_voxel_mask> -number 20000000 <output_tracks_20_million.tck>
    
21. Reduce biases in tractogram densities
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Perform SIFT to reduce tractography biases in the whole-brain tractogram::

    tcksift <input_tracks_20_million.tck> <input_fod_template_image> <output_tracks_2_million_sift.tck> -term_number 2000000
    
22. Perform statistical analysis of FD, FC, and FDC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 You will need to perform a separate analysis for FD, FC and FDC. Statistics is performed using `connectivity-based fixel enhancement <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_ as follows::
 
     fixelcfestats <all_subject_data> <input_files> <input_design_matrix.txt> <output_contrast_matrix.txt> input_tracks_2_million_sift.tck <output_folder>

Where the input files.txt is a text file containing the file path and name of each input fixel file on a separate line. The line ordering should correspond to the lines in the design_matrix.txt. Note that for correlation analysis, a column of 1's will not be automatically included (as per FSL randomise). Note that fixelcfestats currently only accepts a single contrast. However if the opposite (negative) contrast is also required (i.e. a two-tailed test), then use the :code:`-neg` option. Several output files will generated all starting with the supplied prefix.

23. Visualise the results
^^^^^^^^^^^^^^^^^^^^^^^^^
To view the results load the population FOD template image in :code:`mrview`, and overlay the fixel images using the vector plot tool. Note that p-value images are saved as 1-p-value. Therefore to visualise all p-values < 0.05, threshold the fixels using the vector plot tool at 0.95.








