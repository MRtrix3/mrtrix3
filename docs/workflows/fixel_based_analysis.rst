Fixel-Based Analysis (FBA)
==========================

Introduction
-------------

This tutorial explains how to perform fixel-based analysis using MRtrix commands. While the focus here is on the analysis of `Apparent Fibre Density (AFD) <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_ derived from FODs, the majority of the steps in this tutorial are generic and therefore a similar process can be used to investigate other fixel-based measures derived from other diffusion MRI models. 

.. WARNING:: This tutorial assumes you have already pre-processed your data to remove artefacts (e.g. eddy-current and magnetic suseptibility-induced distortions and subject motion). If performing analysis of AFD derived from FODs, then additional pre-processing is required and explained in `this tutorial <http://mrtrix.readthedocs.io/en/latest/workflows/DWI_preprocessing_for_quantitative_analysis.html>`_. Please post any questions or issues on the `MRtrix community forum <http://community.mrtrix.org/>`_.


Fixel-based analysis steps
---------------------------

.. WARNING:: The following steps and commands are pre-release only. It is likely that some command and option names will change over the next few months, however the overall process will remain the same. We recommend you don't update MRtrix half way through a study, and look out for update announcements on the `MRtrix community <http://community.mrtrix.org/>`_ and `blog <www.mrtrix.org/blog/>`_ pages.

Note that for all MRtrix scripts and commands, additional information on the command usage and available command-line options can be found by invoking the command with the :code:`-help` option. 

1. Upsampling DW images
^^^^^^^^^^^^^^^^^^^^^^^
Upsampling DWI data before computing FODs can `increase anatomical contrast <http://www.sciencedirect.com/science/article/pii/S1053811914007472>`_ and improve downstream spatial normalisation and statistics. We recommend upsampling by a factor of two using bspline interpolation::

    mrresize <input_dwi> -scale 2.0 <output_upsampled_dwi>
    
2. Compute upsampled brain mask images
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Compute a whole brain mask from the upsampled DW images::
    
    dwi2mask <input_upsampled_dwi> <output_upsampled_mask>

3. Fibre Orientation Distribution estimation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
This command performs Constrained Spherical Deconvolution (CSD) using the group average response function `estimated previously  <http://userdocs.mrtrix.org/en/latest/workflows/DWI_preprocessing_for_quantitative_analysis.html>`_. Note that :code:`dwi2fod csd` can be used, however here we use :code:`dwi2fod msmt_csd` (even with single shell data) to benefit from the hard non-negativity constraint::

    dwiextract <input_upsampled_dwi> - | dwi2fod msmt_csd - <group_average_response_text_file> <output_fod_image> -mask <input_upsampled_mask>

4. Generate a study-specific unbiased FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Population template creation is the most time consuming step in a fixel-based analysis. If you have a large number of subjects in your study, we recommend building the template from a subset of 20-40 individuals. Subjects should be chosen to ensure the generated template is representative of your population (i.e. equal number of patients and controls). To build a template, place all FOD images in a single folder. We also recommend placing a set of corresponding mask images (with the same prefix as the FOD images) in another folder. Using masks can speed up registration significantly. Run the population_template building script as follows::
    
    population_template <input_folder_of_FOD_images> -mask_dir <input_mask_folder> <output_fod_template_image>

.. NOTE::If you are building a template from your entire study population use the -warp_dir option to output a folder containing all subject warps to the template. Saving the warps here will enable you to skip the next step. 

5. Register all subject FOD images to the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Register the FOD image from all subjects to the FOD template image::

    mrregister <input_fod_image> -mask1 <input_subject_mask> <input_fod_template_image> -nl_warp <subject2template_warp> <template2subject_warp>


6. Compute the intersection of all subject masks in template space
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Different subjects will have subtly different brain coverage. To ensure subsequent analysis is performed in voxels that contain data from all subjects, we warp all subject masks into template space and compute the mask intersection. For each subject::
    
    mrtransform <input_upsampled_mask_image> -warp <subject2template_warp> -interp nearest <output_warped_mask>

Compute the intersection of all warped masks::
    
    mrmath <input_all_warped_masks_multiple_inputs> min <output_template_mask_intersection>
    
    
7. Compute a white matter template analysis fixel mask     
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Here we perform a 2-step threshold to identify template white matter fixels to be included in the analysis. Fixels in the template fixel analysis mask are also used to identify the best fixel correspondence across all subjects (i.e. match fixels across subjects within a voxel). 
       
Compute a template AFD peaks fixel image::
    
    fod2fixel <input_fod_template_image> -mask <input_template_mask_intersection> -peak <template_peaks_image.msf> 
    
.. NOTE:: Fixel images in MRtrix must be stored using the .msf (MRtrix sparse format) extension. 
    
Next view the peaks file using the vector plot tool in mrview and identify an appropriate threshold that removes peaks from grey matter, yet does not introduce any 'holes' in your white matter (approximately 0.33).      

Threshold the peaks fixel image::
    
    fixelthreshold -crop <template_peaks_image.msf> 0.33 <analysis_fixel_mask.msf>

Generate an analysis voxel mask from the fixel mask. The median filter in this step should remove spurious voxels outside the brain, and fill in the holes in deep white matter where you have small peaks due to 3-fibre crossings::

    fixel2voxel <analysis_fixel_mask.msf> count - | mrthreshold - - -abs 0.5 | mrfilter - median <output_analysis_voxel_mask>

Recompute the fixel mask using the analysis voxel mask. Using the mask allows us to use a lower AFD threshold than possible in the steps above, to ensure we have included fixels with low AFD inside white matter::
 
    fod2fixel -mask <input_analysis_voxel_mask> <input_fod_template_image> -peak <output_temp.msf>
    fixelthreshold <input_temp.msf> -crop 0.2 <output_analysis_fixel_mask.msf> -force
    rm <temp.msf>
    
.. NOTE:: We recommend having no more than 500,000 fixels in the analysis_fixel_mask (you can check this with :code:`fixelstats`), otherwise downstream statistical analysis (using :code:`fixelcfestats`) will run out of RAM). A mask with 500,000 fixels will require a PC with 128GB of RAM for the statistical analysis step.

8. Transform FOD images to template space
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Note that here we transform FOD images into template space *without* FOD reorientation. Reorientation will be performed in a separate subsequent step:: 

    mrtransform <input_subject_fod_image> -warp <subject2template_warp> -noreorientation <output_warped_fod_image>

9. Segment FOD images to estimate fixels and their fibre density (FD)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Here we segment each FOD lobe to identify the number and orientation of fixels in each voxel. The output also contains the apparent fibre density (AFD) value per fixel estimated as the FOD lobe integral (see `here <http://www.sciencedirect.com/science/article/pii/S1053811912011615>`_ for details on FOD segmentation). Note that in the following steps we will use a more generic shortened acronym - Fibre Density (FD) instead of AFD for consistency with our recent work (paper under review)::

    fod2fixel <input_warped_fod_image> -mask <input_analysis_voxel_mask> -afd <output_fd_not_reoriented.msf>
    
.. NOTE:: If you would like to perform fixel-based analysis of metrics derived from other diffusion MRI models (e.g. CHARMED), replace steps 8 & 9. For example, in step 8 you can warp preprocessed DW images (also without any reorientation). In step 9 you could then estimate your DWI model of choice. 
    
    
10. Reorient fixel orientations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Here we reorient the direction of all fixels based on the Jacobian matrix (local affine transformation) at each voxel in the warp::

    fixelreorient <input_fd_not_reoriented.msf> <subject2template_warp> <output_fd_reoriented.msf>
    
11. Assign subject fixels to template fixels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In step 8 we obtained spatial correspondence between subject and template. In step 10 we corrected the fixel orientations to ensure angular correspondence of the segmented peaks of subject and template. Here, for each fixel in the template fixel analysis mask, we identify the corresponding fixel in each voxel of the subject image and assign the FD value of the subject fixel to the corresponding fixel in template space. If no fixel exists in the subject that corresponds to the template fixel then it is assigned a value of zero. See `this paper <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_ for more information::

    fixelcorrespondence <input_fd_reoriented.msf> <input_analysis_fixel_mask.msf> <output_fd.msf>
    
12. Compute fibre cross-section (FC) metric
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Apparent fibre density, and other related measures that are influenced by the quantity of restricted water, only permit the investigation of group differences in the number of axons that manifest as a change to *within-voxel* density. However, depending on the disease type and stage, changes to the number of axons may also manifest as macroscopic differences in brain morphology. This step computes a fixel-based metric related to morphological differences in fibre cross-section, where information is derived entirely from the warps generated during registration (paper under review):: 

    warp2metric <subject2template_warp> -fc <input_analysis_fixel_mask.msf> <output_fc.msf>
    
The FC files will be used in the next step. However, for group statistical analysis of FC we recommend taking the log (FC) to ensure data are centred about zero and normally distributed::

    fixellog <input_fc.msf> <output_log_fc.msf>

13. Compute a combined measure of fibre density and cross-section (FDC)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To account for changes to both within-voxel fibre density and macroscopic atrophy, fibre density and fibre cross-section must be combined (a measure we call fibre density & cross-section, FDC). This enables a more complete picture of group differences in white matter. Note that as discussed in our future work (under review), group differences in FD or FC alone must be interpreted with care in crossing-fibre regions. However group differences in FDC are more directly interpretable. To generate the combined measure we 'modulate' the FD by FC::

    fixelcalc <input_fd.msf> mult <input_fc.msf> <output_fdc.msf>
    
14. Perform whole-brain fibre tractography on the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Statistical analysis using `connectivity-based fixel enhancement <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_ exploits connectivity information derived from probabilistic fibre tractography. To generate a whole-brain tractogram from the FOD template::
    
    tckgen -angle 22.5 -maxlen 250 -minlen 10 -power 1.0 <input_fod_template_image> -seed_image <input_analysis_voxel_mask> -mask <input_analysis_voxel_mask> -number 20000000 <output_tracks_20_million.tck>
    
15. Reduce biases in tractogram densities
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Perform SIFT to reduce tractography biases in the whole-brain tractogram::

    tcksift <input_tracks_20_million.tck> <input_fod_template_image> <output_tracks_2_million_sift.tck> -term_number 2000000
    
16. Perform statistical analysis of FD, FC, and FDC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 You will need to perform a separate analysis for FD, FC and FDC. Statistics is performed using `connectivity-based fixel enhancement <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_ as follows::
 
     fixelcfestats <input_files> <input_analysis_fixel_mask.msf> <input_design_matrix.txt> <output_contrast_matrix.txt> <input_tracks_2_million_sift.tck> <output_prefix>
  
Where the input files.txt is a text file containing the file path and name of each input fixel file on a separate line. The line ordering should correspond to the lines in the design_matrix.txt. Note that for correlation analysis, a column of 1's will not be automatically included (as per FSL randomise). Note that fixelcfestats currently only accepts a single contrast. However if the opposite (negative) contrast is also required (i.e. a two-tailed test), then use the :code:`-neg` option. Several output files will generated all starting with the supplied prefix.

17. Visualise the results 
^^^^^^^^^^^^^^^^^^^^^^^^^
To view the results load the population FOD template image in :code:`mrview`, and overlay the fixel images using the vector plot tool. Note that p-value images are saved as 1-p-value. Therefore to visualise all p-values < 0.05, threshold the fixels using the vector plot tool at 0.95.








