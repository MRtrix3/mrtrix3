Fibre Density and Cross-section - Multi-Tissue DWI
==================================================

Introduction
-------------

This tutorial explains how to perform `fixel-based analysis of fibre density and cross-section <https://www.ncbi.nlm.nih.gov/pubmed/27639350>`_ with fibre orientation distributions (FODs) computing using multi-tissue CSD using `single-shell <https://www.researchgate.net/publication/301766619_A_novel_iterative_approach_to_reap_the_benefits_of_multi-tissue_CSD_from_just_single-shell_b0_diffusion_MRI_data>`_ data or `multi-shell data <https://www.ncbi.nlm.nih.gov/pubmed/25109526>`_. While the focus here is on the analysis of `Apparent Fibre Density (AFD) <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_ derived from FODs, other fixel-based measures related to fibre density can also be analysed with a few minor modifications to these steps (as outlined below). We note that high b-value (>2000s/mm :sup: `2`) data is recommended to aid the interpretation of AFD being related to the intra-axonal space. See the `original paper <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_ for more details.


All steps in this tutorial have written as if the commands are being **run on a cohort of images**, and make extensive use of the :ref:`foreach script to simplify batch processing <batch_processing>`. This tutorial also assumes that the imaging dataset is organised with one directory identifying the subject, and all files within identifying the image type. For example::

    study/subjects/001_patient/dwi.mif
    study/subjects/001_patient/t1.mif
    study/subjects/002_control/dwi.mif
    study/subjects/002_control/t1.mif

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

    foreach * : dwi2response dhollander IN/dwi_denoised_preproc_bias_norm.mif IN/response_wm.txt IN/response_gm.txt IN/response_csf.txt
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

    foreach * : dwi2mask IN/dwi_denoised_preproc.mif - \| mrresize - -scale 2.0 -inter nearest IN/dwi_mask_upsampled.mif


9. Fibre Orientation Distribution estimation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When performing analysis of AFD, Constrained Spherical Deconvolution (CSD) should be performed using the group average response function computed at step . If not using AFD in the fixel-based analysis (and therefore you have skipped steps 4-6), however you still want to compute FODs for image registration, then you can use a subject-specific response function. Note that :code:`dwi2fod csd` can be used, however here we use :code:`dwi2fod msmt_csd` (even with single shell data) to benefit from the hard non-negativity constraint::

    foreach * : dwiextract IN/dwi_denoised_preproc_bias_norm_upsampled.mif - \| dwi2fod msmt_csd - ../group_average_response.txt IN/fod.mif -mask IN/dwi_mask_upsampled.mif

10. Generate a study-specific unbiased FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/population_template.rst

11. Register all subject FOD images to the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/register_to_template.rst

12. Compute the intersection of all subject masks in template space
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/mask_intersection.rst


13. Compute a white matter template analysis fixel mask
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Here we perform a 2-step threshold to identify template white matter fixels to be included in the analysis. Fixels in the template fixel analysis mask are also used to identify the best fixel correspondence across all subjects (i.e. match fixels across subjects within a voxel).

Compute a template AFD peaks fixel image::

    fod2fixel ../template/fod_template.mif -mask ../template/mask_intersection.mif ../template/fixel_template_temp -peak peaks.mif

.. NOTE:: Fixel images in this step are stored using the :ref:`fixel_format`, which exploits the filesystem to store all fixel data in a directory.

Next view the peaks file using the fixel plot tool in :ref:`mrview` and identify an appropriate threshold that removes peaks from grey matter, yet does not introduce any 'holes' in your white matter (approximately 0.33).

Threshold the peaks fixel image::

    mrthreshold ../template/fixel_template_temp/peaks.mif -abs 0.33 ../template/fixel_template_temp/mask.mif

Generate an analysis voxel mask from the fixel mask. The median filter in this step should remove spurious voxels outside the brain, and fill in the holes in deep white matter where you have small peaks due to 3-fibre crossings::

    fixel2voxel ../template/fixel_template_temp/mask.mif count - | mrthreshold - - -abs 0.5 | mrfilter - median ../template/voxel_mask.mif

Recompute the fixel mask using the analysis voxel mask. Using the mask allows us to use a lower AFD threshold than possible in the steps above, to ensure we have included fixels with low AFD inside white matter::

    fod2fixel -mask ../template/voxel_mask.mif ../template/fod_template.mif ../template/fixel_template/ -peak peaks.mif
    mrthreshold ../template/fixel_template/peaks.mif -abs 0.2 ../template/fixel_template/mask.mif

You can now remove the fixel mask from the intermediate step to avoid confusion later::

    rm -rf ../template/fixel_template_temp

.. NOTE:: We recommend having no more than 500,000 fixels in the analysis_fixel_mask (you can check this by :code:`mrinfo -size ../template/fixel_template/mask.mif`, and looking at the size of the image along the 1st dimension), otherwise downstream statistical analysis (using :ref:`fixelcfestats`) will run out of RAM). A mask with 500,000 fixels will require a PC with 128GB of RAM for the statistical analysis step.

14. Warp FOD images to template space
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/warp_fods.rst

15. Segment FOD images to estimate fixels and their apparent fibre density (FD)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_AFD.rst

16. Reorient fixel orientations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/reorient_fixels.rst

17. Assign subject fixels to template fixels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In step 10 & 11 we obtained spatial correspondence between subject and template. In step 16 we corrected the fixel orientations to ensure angular correspondence of the segmented peaks of subject and template. Here, for each fixel in the template fixel analysis mask, we identify the corresponding fixel in each voxel of the subject image and assign the FD value of the subject fixel to the corresponding fixel in template space. If no fixel exists in the subject that corresponds to the template fixel then it is assigned a value of zero. See `this paper <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_ for more information. In the command below, you will note that the output fixel directory is the same for all subjects. This directory now stores data for all subjects at corresponding fixels, ready for input to :code:`fixelcfestats` in step 22 below::

    foreach * : fixelcorrespondence IN/fixel_in_template_space/fd.mif ../template/fixel_template ../template/fd PRE.mif

18. Compute fibre cross-section (FC) metric
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_FC.rst

19. Compute a combined measure of fibre density and cross-section (FDC)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_FDC.rst

20. Perform whole-brain fibre tractography on the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/tractography.rst

21. Reduce biases in tractogram densities
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/sift.rst

22. Perform statistical analysis of FD, FC, and FDC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/statistics.rst


23. Visualise the results
^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/visualisation.rst








