Fixel-Based Analysis (FBA)
==========================

Introduction
-------------

This tutorial explains how to perform fixel-based analysis using MRtrix commands. While the focus here is on the analysis of `Apparent Fibre Density (AFD) <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_ derived from FODs, the majority of the steps in this tutorial are generic and therefore a similar process can be used to investigate other fixel-based measures derived from other diffusion MRI models. 

.. WARNING:: This tutorial assumes you have already pre-processed your data to remove artefacts (e.g. eddy-current and magnetic suseptibility-induced distortions and subject motion). If performing analysis of AFD derived from FODs, then additional pre-processing is required and explained in `this tutorial <http://userdocs.mrtrix.org/en/latest/workflows/DWI_preprocessing_for_quantitative_analysis.html>`_. Please post any questions or issues on the `MRtrix community forum <http://community.mrtrix.org/>`_.

Fixel-based analysis steps
---------------------------
Below is a list of steps for fixel-based analysis using FOD images. Note that for all MRtrix scripts and commands, additional information on the command usage and available command-line options can be found by invoking the command with the :code:`-help` option. 

1. Upsampling DW images
^^^^^^^^^^^^^^^^^^^^^^^
Upsampling DWI data before computing FODs can `increase anatomical contrast <http://www.sciencedirect.com/science/article/pii/S1053811914007472>`_ and improve downstream spatial normalisation and statistics. We recommend upsampling by a factor of two using bspline interpolation::
    mrresize <input_dwi> -scale 2.0 <output_upsampled_dwi>
    
2. Compute upsampled brain mask images
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
::
    dwi2mask <input_upsampled_dwi> <output_upsampled_mask>

3. Fibre Orientation Distribution estimation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
This command performs Constrained Spherical Deconvolution (CSD) using the group average response function `estimated previously  <http://userdocs.mrtrix.org/en/latest/workflows/DWI_preprocessing_for_quantitative_analysis.html>`_::

    dwi2fod <input_upsampled_dwi> <group_average_response_text_file> <output_fod_image> -mask <input_upsampled_mask>

4. Generate a study-specific unbiased FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Population template creation is the most time consuming step in a fixel-based analysis. If you have a large number of subjects in your study, we recommend building the template from a subset of 20-40 individuals. Subjects should be chosen to ensure the generated template is representative of your population (i.e. equal number of patients and controls). To build a template, place all FOD images in a single folder. We also recommend placing a set of corresponding mask images (with the same prefix as the FOD images) in another folder. Using masks can speed up registration significantly. Run the population_template building script as follows::
    population_template <input_folder_of_FOD_images> -mask_dir <input_mask_folder> <output_fod_template_image>

.. NOTE::If you are building a template from your entire study population use the -warp_dir option to output a folder containing all subject warps to the template. Saving the warps here will enable you to skip the next step. 

5. Register all subject FOD images to the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Register the FOD image from all subjects to the FOD template image::
    mrregister <input_fod_image> -mask1 <input_subject_mask> <input_fod_template_image> -warp <subject2template_warp> <template2subject_warp>


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

Generate a analysis voxel mask from the fixel mask. The median filter in this step should remove spurious voxels outside the brain, and fill in the holes in deep white matter where you have small peaks due to 3-fibre crossings:::
    fixel2voxel <analysis_fixel_mask.msf> count - | mrthreshold - - -abs 0.5 | mrfilter - median <output_analysis_voxel_mask>

Recompute the fixel mask using the analysis voxel mask. Using the mask allows us to use a lower AFD threshold than possible in the steps above, to ensure we have included fixels with low AFD inside white matter
 .. code:: bash

    fod2fixel -mask <input_analysis_voxel_mask> <input_fod_template_image> -peak <output_temp.msf>
    fixelthreshold <input_temp.msf> -crop 0.2 <output_analysis_fixel_mask.msf> -force
    rm <temp.msf>

8. Transform FOD images to template space
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Note that here we transform FOD images into template space without FOD reorientation. Reorientation will be performed in a seperate subsequent step. 
mrtransform <input_subject_fod_image> -warp <subject2template_warp> -noreorientation <output_warped_fod_image>

9. Segment FOD images to estimate fixels and their AFD integral
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Here we segment each FOD lobe to identify the number and orientation of fixels in each voxel. The output also contains the AFD value per fixel estimated as the FOD lobe integral (see `here <http://www.sciencedirect.com/science/article/pii/S1053811912011615>`_ for details on FOD segmentation)::
    fod2fixel <input_warped_fod_image> -mask ../2S3Tcsd8_voxel_analysis_mask.mif -afd <output_fd.msf>







