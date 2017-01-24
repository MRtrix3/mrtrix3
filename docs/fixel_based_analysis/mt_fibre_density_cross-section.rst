Fibre Density and Cross-section - Multi-Tissue CSD
==================================================

Introduction
-------------

This tutorial explains how to perform `fixel-based analysis of fibre density and cross-section <https://www.ncbi.nlm.nih.gov/pubmed/27639350>`_ with fibre orientation distributions (FODs) computing using multi-tissue CSD using `single-shell <https://www.researchgate.net/publication/301766619_A_novel_iterative_approach_to_reap_the_benefits_of_multi-tissue_CSD_from_just_single-shell_b0_diffusion_MRI_data>`_ data or `multi-shell data <https://www.ncbi.nlm.nih.gov/pubmed/25109526>`_. We note that high b-value (>2000s/mm2) data is recommended to aid the interpretation of AFD being related to the intra-axonal space. See the `original paper <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_ for more details.


All steps in this tutorial have written as if the commands are being **run on a cohort of images**, and make extensive use of the :ref:`foreach script to simplify batch processing <batch_processing>`. This tutorial also assumes that the imaging dataset is organised with one directory identifying the subject, and all files within identifying the image type. For example::

    study/subjects/001_patient/dwi.mif
    study/subjects/001_patient/fod.mif
    study/subjects/002_control/dwi.mif
    study/subjects/002_control/fod.mif

.. NOTE:: All commands in this tutorial are run **from the subjects path** up until step 18, where we change directory to the template path

For all MRtrix scripts and commands, additional information on the command usage and available command-line options can be found by invoking the command with the :code:`-help` option. Please post any questions or issues on the `MRtrix community forum <http://community.mrtrix.org/>`_.


Pre-processsing steps
---------------------

1. DWI denoising
^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/dwidenoise.rst

2. DWI general pre-processing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/dwipreproc.rst


Fixel-based analysis steps
---------------------------

3. Computing group average tissue response functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
As described `here <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_, using the same response function when estimating FOD images for all subjects enables differences in the intra-axonal volume (and therefore DW signal) across subjects to be detected as differences in the FOD amplitude (the AFD). To ensure the response function is representative of your study population, a group average response function can be computed by first estimating a response function per subject, then averaging with the script::

    foreach * : dwi2response dhollander IN/dwi_denoised_preproc.mif IN/response_wm.txt IN/response_gm.txt IN/response_csf.txt
    average_response */response_wm.txt ../group_average_response_wm.txt
    average_response */response_gm.txt ../group_average_response_gm.txt
    average_response */response_csf.txt ../group_average_response_csf.txt


4. Upsampling DW images
^^^^^^^^^^^^^^^^^^^^^^^

Upsampling DWI data before computing FODs can `increase anatomical contrast <http://www.sciencedirect.com/science/article/pii/S1053811914007472>`_ and improve downstream spatial normalisation and statistics. We recommend upsampling by a factor of two using bspline interpolation. Note that if you already have higher than normal DWI resolution (e.g. HCP data), then we recommend you skip this step::

    foreach * : mrresize IN/dwi_denoised_preproc.mif -scale 2.0 IN/dwi_denoised_preproc_upsampled.mif


5. Compute upsampled brain mask images
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Compute a whole brain mask from the upsampled DW images::

    foreach * : dwi2mask IN/dwi_denoised_preproc_upsampled.mif IN/dwi_mask_upsampled.mif

Depending on your data, you may find that computing masks on native resolution DWIs gives superiour masks (with less holes), then upsampling. This can be performed using::

    foreach * : dwi2mask IN/dwi_denoised_preproc.mif - \| mrresize - -scale 2.0 -interp nearest IN/dwi_mask_upsampled.mif


6. Fibre Orientation Distribution estimation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When performing analysis of AFD, Constrained Spherical Deconvolution (CSD) should be performed using the group average response functions computed at step 3.

    foreach * : dwi2fod msmt_csd IN/dwi_denoised_preproc_upsampled.mif ../group_average_response_wm.txt IN/fod.mif ../group_average_response_gm.txt IN/gm.mif  ../group_average_response_csf.txt IN/csf.mif -mask IN/dwi_mask_upsampled.mif


7. Perform simultaneous bias field correction and intensity normalisation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
This step performs :ref:`global intensity normalisation <global-intensity-normalisation>` by scaling all tissue types based on a single scale factor. A single multiplicative bias field is also estimated and applied to correct the output::

    foreach * : mtbin IN/fod.mif IN/fod_bias_norm.mif IN/gm.mif IN/gm_bias_norm.mif IN/csf.mif IN/csf_bias_norm.mif

.. WARNING:: We strongly recommend you that you check the scale factors applied during intensity normalisation are not influenced by the variable of interest in your study. For example if one group contains global changes in white matter T2 then this may directly influence the intensity normalisation and therefore bias downstream AFD analysis. To check this we recommend you perform an equivalence test to ensure mean scale factors are the same between groups. To output the scale factor applied for all subjects use :code:`foreach * : mrinfo IN/fod_bias_norm.mif -property normalisation_scale_factor`.


8. Generate a study-specific unbiased FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/population_template.rst

    foreach * : ln -sr IN/fod_wm_bias_norm.mif ../template/fod_input/PRE.mif
    foreach * : ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif

Alternatively, if you have more than 40 subjects you can randomly select a subset of the individuals. If your study has multiple groups, then ideally you want to select the same number of subjects from each group to ensure the template is un-biased. Assuming the subject directory labels can be used to identify members of each group, you could use::

    foreach `ls -d *patient | sort -R | tail -20` : ln -sr IN/fod_wm_bias_norm.mif ../template/fod_input/PRE.mif ";" ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif
    foreach `ls *control | sort -R | tail -20` : ln -sr IN/fod_wm_bias_norm.mif ../template/fod_input/PRE.mif ";" ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif

.. include:: common_fba_steps/population_template2.rst

9. Register all subject FOD images to the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Register the FOD image from all subjects to the FOD template image. Note you can skip this step if you built your template from your entire population and saved the warps (see previous step)::

    foreach * : mrregister IN/fod_wm_bias_norm.mif -mask1 IN/dwi_mask_upsampled.mif ../template/fod_template.mif -nl_warp IN/subject2template_warp.mif IN/template2subject_warp.mif

10. Compute the intersection of all subject masks in template space
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/mask_intersection.rst


11. Compute a white matter template analysis fixel mask
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Next we identify all fixels of interest to be analysed from the FOD template image (see `here <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_ for more information about a fixel analysis mask). Note that the fixel image output from this step is stored using the :ref:`fixel_format`, which exploits the filesystem to store all fixel data in a directory::

    mrconvert fod_template.mif -coord 3 0 - | mrthreshold - - | fod2fixel fod_template.mif -mask - ../template/fixel_template

You can visualise the output fixels using the fixel plot tool from :ref:`mrview`, and opening either the :code:`index.mif` or :code:`directions.mif` found in :code:`../template/fixel_template`. The automatic thresholding step used above should give you a mask that nicely covers all of white matter, however if not you can always try manually adjusting the threshold with the :code:`mrthreshold -abs` option.

.. NOTE:: We recommend having no more than 500,000 fixels in the analysis_fixel_mask (you can check this by :code:`mrinfo -size ../template/fixel_template/directions.mif`, and looking at the size of the image along the 1st dimension), otherwise downstream statistical analysis (using :ref:`fixelcfestats`) will run out of RAM). A mask with 500,000 fixels will require a PC with 128GB of RAM for the statistical analysis step.

12. Warp FOD images to template space
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Note that here we warp FOD images into template space *without* FOD reorientation. Reorientation will be performed in a separate subsequent step::

    foreach * : mrtransform IN/fod_wm_bias_norm.mif -warp IN/subject2template_warp.mif -noreorientation IN/fod_in_template_space.mif


13. Segment FOD images to estimate fixels and their apparent fibre density (FD)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_AFD.rst


14. Reorient fixel orientations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/reorient_fixels.rst

15. Assign subject fixels to template fixels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In step 8 & 9 we obtained spatial correspondence between subject and template. In step 14 we corrected the fixel orientations to ensure angular correspondence of the segmented peaks of subject and template. Here, for each fixel in the template fixel analysis mask, we identify the corresponding fixel in each voxel of the subject image and assign the FD value of the subject fixel to the corresponding fixel in template space. If no fixel exists in the subject that corresponds to the template fixel then it is assigned a value of zero. See `this paper <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_ for more information. In the command below, you will note that the output fixel directory is the same for all subjects. This directory now stores data for all subjects at corresponding fixels, ready for input to :code:`fixelcfestats` in step 20 below::

    foreach * : fixelcorrespondence IN/fixel_in_template_space/fd.mif ../template/fixel_template ../template/fd PRE.mif

16. Compute fibre cross-section (FC) metric
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_FC.rst

17. Compute a combined measure of fibre density and cross-section (FDC)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_FDC.rst

18. Perform whole-brain fibre tractography on the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/tractography.rst

19. Reduce biases in tractogram densities
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/sift.rst

20. Perform statistical analysis of FD, FC, and FDC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/statistics.rst

21. Visualise the results
^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/visualisation.rst








