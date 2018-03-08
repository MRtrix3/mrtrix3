Fibre density and cross-section - Multi-tissue CSD
==================================================

Introduction
-------------

This tutorial explains how to perform `fixel-based analysis of fibre density and cross-section <https://www.ncbi.nlm.nih.gov/pubmed/27639350>`_ with fibre orientation distributions (FODs) computing using multi-tissue CSD using `single-shell <https://www.researchgate.net/publication/301766619_A_novel_iterative_approach_to_reap_the_benefits_of_multi-tissue_CSD_from_just_single-shell_b0_diffusion_MRI_data>`_ data or `multi-shell data <https://www.ncbi.nlm.nih.gov/pubmed/25109526>`_. We note that high b-value (>2000s/mm2) data is recommended to aid the interpretation of apparent fibre density (AFD) being related to the intra-axonal space. See this `paper <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_ for more details.

All steps in this tutorial have written as if the commands are being **run on a cohort of images**, and make extensive use of the :ref:`foreach script to simplify batch processing <batch_processing>`. This tutorial also assumes that the imaging dataset is organised with one directory identifying the subject, and all files within identifying the image type. For example::

    study/subjects/001_patient/dwi.mif
    study/subjects/001_patient/wmfod.mif
    study/subjects/002_control/dwi.mif
    study/subjects/002_control/wmfod.mif

.. NOTE:: All commands at the start of this tutorial are run **from the subjects path**. From the step where tractography is performed on the template onwards, we change directory **to the template path**.

For all MRtrix scripts and commands, additional information on the command usage and available command-line options can be found by invoking the command with the :code:`-help` option.


Pre-processsing steps
---------------------

1. Denoising and unringing
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/denoise_unring.rst

2. Motion and distortion correction
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/dwipreproc.rst

3. Bias field correction
^^^^^^^^^^^^^^^^^^^^^^^^
The multi-tissue FBA pipeline corrects for bias fields (and jointly performs global intensity normalisation) at the later :ref:`mtnormalise` step. The only incentive for running the (less robust and accurate) :ref:`dwibiascorrect` at this stage in the pipeline is *to improve brain mask estimation* (at the later :ref:`dwi2mask` step, in case severe bias fields are present in the data). However, cases have been reported where running :ref:`dwibiascorrect` at this stage resulted in *inferior* brain mask estimation later on. This is probably more likely in case bias fields are not as strongly present in the data. Whether :ref:`dwibiascorrect` is run at this stage or not, does not have any significant impact on the performance of :ref:`mtnormalise` later on.

If or when performing DWI bias field correction at this stage, it is achieved by first estimating the bias field from the DWI b=0 data, then applying the field to correct all DW volumes, which is done in a single step using the :ref:`dwibiascorrect` script in MRtrix. The script uses bias field correction algorthims available in `ANTS <http://stnava.github.io/ANTs/>`_ or `FSL <http://fsl.fmrib.ox.ac.uk/>`_. In our experience the `N4 algorithm <http://www.ncbi.nlm.nih.gov/pmc/articles/PMC3071855/>`_ in ANTS performs better at this task. To install N4, install the `ANTS <http://stnava.github.io/ANTs/>`_ package. To perform bias field correction on DW images, run::

    foreach * : dwibiascorrect -ants IN/IN/dwi_denoised_unringed_preproc.mif IN/dwi_denoised_unringed_preproc_unbiased.mif


Fixel-based analysis steps
--------------------------

4. Computing (average) tissue response functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
A robust and fully automated (unsupervised) method to obtain 3-tissue response functions representing single-fibre white matter, grey matter and CSF from your data, is the approach proposed in `Dhollander et al. (2016) <https://www.researchgate.net/publication/307863133_Unsupervised_3-tissue_response_function_estimation_from_single-shell_or_multi-shell_diffusion_MR_data_without_a_co-registered_T1_image>`__, which can be run by::

    foreach * : dwi2response dhollander IN/dwi_denoised_unringed_preproc_unbiased.mif IN/response_wm.txt IN/response_gm.txt IN/response_csf.txt

It is crucial for fixel-based analysis to only use a single *unique* set of the (three) response functions to perform (3-tissue) spherical deconvolution of all subjects: as the (3-tissue) spherical deconvolution results will be expressed in function of this set of response functions, they can (in an abstract way) be seen as the units of both the final apparent fibre density metric and the other compartments estimated in the model. A possible way to obtain a unique set of response functions, is to average the response functions obtained from all subjects for each tissue type::

    average_response */response_wm.txt ../group_average_response_wm.txt
    average_response */response_gm.txt ../group_average_response_gm.txt
    average_response */response_csf.txt ../group_average_response_csf.txt

There is however no strict requirement that the final set of response functions is the average of *all* subject response functions (for each tissue type). In certain very specific cases, it may even be wise to leave out subjects (for this step) where the response functions could not reliably be obtained, or where pathology affected the brain globally.

5. Upsampling DW images
^^^^^^^^^^^^^^^^^^^^^^^
Upsampling DWI data *before* computing FODs increases anatomical contrast and improves downstream template building, registration, tractography and statistics. We recommend upsampling to an isotropic voxel size of 1.3 mm for human brains (if your original resolution is already higher, you can skip this step)::

    foreach * : mrresize IN/dwi_denoised_unringed_preproc_unbiased.mif -vox 1.3 IN/dwi_denoised_unringed_preproc_unbiased_upsampled.mif


6. Compute upsampled brain mask images
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Compute a whole brain mask from the upsampled DW images::

    foreach * : dwi2mask IN/dwi_denoised_unringed_preproc_unbiased_upsampled.mif IN/dwi_mask_upsampled.mif

.. WARNING:: It is absolutely **crucial** to check at this stage that *all* individual subject masks include *all* areas of the brain that are intended to be analysed. Fibre orientation distributions will *only* be computed within these masks; and at a later step (in template space) the analysis mask will be restricted to the *intersection* of all masks, so *any* individual subject mask which excludes a certain area, will result in the area being excluded from the entire analysis. Masks appearing too generous or otherwise including non-brain areas should generally not cause any concerns at this stage. Hence, if in doubt, it is advised to always err on the side of *inclusion* (of areas) at this stage.

.. NOTE:: The earlier :ref:`dwibiascorrect` step is not fundamentally important in the multi-tissue fixel-based analysis pipeline, as the later :ref:`mtnormalise` step performs more robustly (and if :ref:`dwibiascorrect` is included, :ref:`mtnormalise` will later on typically improve the result further). While performing the earlier :ref:`dwibiascorrect` step typically improves :ref:`dwi2mask` performance, cases have been observed where the opposite is true (typically if the data contained only weak bias field induced intensity inhomogeneities). Experiment with either including or excluding :ref:`dwibiascorrect` in function of the best :ref:`dwi2mask` outcome and manually correct the masks if necessary (by *including* regions which :ref:`dwi2mask` fails to include).

7. Fibre Orientation Distribution estimation (multi-tissue spherical deconvolution)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When performing fixel-based analysis, multi-tissue constrained spherical deconvolution should be performed using the unique set of (average) response functions obtained before::

    foreach * : dwi2fod msmt_csd IN/dwi_denoised_unringed_preproc_unbiased_upsampled.mif ../group_average_response_wm.txt IN/wmfod.mif ../group_average_response_gm.txt IN/gm.mif  ../group_average_response_csf.txt IN/csf.mif -mask IN/dwi_mask_upsampled.mif


8. Joint bias field correction and intensity normalisation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This step performs :ref:`global intensity normalisation <global-intensity-normalisation>` in the log-domain by scaling all tissue types with a spatially smoothly varying normalisation field::

    foreach * : mtnormalise IN/wmfod.mif IN/wmfod_norm.mif IN/gm.mif IN/gm_norm.mif IN/csf.mif IN/csf_norm.mif -mask IN/dwi_mask_upsampled.mif

If CSD was performed with the same single set of (average) WM, GM and CSF response functions for all subjects, then the resulting output of :ref:`mtnormalise` should make the amplitudes comparable between those subjects as well.

Note that this step is crucial in the FBA pipeline, even if bias field correction was applied during the preprocessing stage, as the latter does not correct for global intensity differences between subjects.


9. Generate a study-specific unbiased FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/population_template.rst

Symbolic link all FOD images (and masks) into a single input folder. If you have fewer than 40 subjects in your study, you can use the entire population to build the template::

    foreach * : ln -sr IN/wmfod_norm.mif ../template/fod_input/PRE.mif
    foreach * : ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif

Alternatively, if you have more than 40 subjects you can randomly select a subset of the individuals. If your study has multiple groups, then ideally you want to select the same number of subjects from each group to ensure the template is un-biased. Assuming the subject directory labels can be used to identify members of each group, you could use::

    foreach `ls -d *patient | sort -R | tail -20` : ln -sr IN/wmfod_norm.mif ../template/fod_input/PRE.mif ";" ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif
    foreach `ls -d *control | sort -R | tail -20` : ln -sr IN/wmfod_norm.mif ../template/fod_input/PRE.mif ";" ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif

.. include:: common_fba_steps/population_template2.rst

10. Register all subject FOD images to the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Register the FOD image from all subjects to the FOD template image. Note you can skip this step if you built your template from your entire population and saved the warps (see previous step)::

    foreach * : mrregister IN/wmfod_norm.mif -mask1 IN/dwi_mask_upsampled.mif ../template/wmfod_template.mif -nl_warp IN/subject2template_warp.mif IN/template2subject_warp.mif

11. Compute the template mask (intersection of all subject masks in template space)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/template_mask.rst


12. Compute a white matter template analysis fixel mask
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

We segment all fixels from each FOD in the template image (see `here <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`__ for more information about a white matter template analysis fixel mask). Note that the fixel image output from this step is stored using the :ref:`fixel_format`, which exploits the filesystem to store all fixel data in a directory::

   fod2fixel -mask ../template/template_mask.mif -fmls_peak_value 0.06 ../template/wmfod_template.mif ../template/fixel_mask

You can visualise the output fixels using the fixel plot tool from :ref:`mrview`, and opening either the :code:`index.mif` or :code:`directions.mif` found in :code:`../template/fixel_mask`. The automatic thresholding step used above should give you a mask that nicely covers all of white matter, however if not you can always try manually adjusting the threshold with the :code:`mrthreshold -abs` option.

.. NOTE:: We recommend having no more than 500,000 fixels in the analysis_fixel_mask (you can check this by :code:`mrinfo -size ../template/fixel_mask/directions.mif`, and looking at the size of the image along the 1st dimension), otherwise downstream statistical analysis (using :ref:`fixelcfestats`) may run out of RAM). A mask with 500,000 fixels will require a PC with 128GB of RAM for the statistical analysis step. To reduce the number of fixels, try either reducing the number of voxels in the voxel mask by applying a manual threshold using :code:`-abs`, increasing the :code:`-fmls_peak_value`, or reducing the extent of upsampling in step 4.

13. Warp FOD images to template space
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Note that here we warp FOD images into template space *without* FOD reorientation. Reorientation will be performed in a separate subsequent step::

    foreach * : mrtransform IN/wmfod_norm.mif -warp IN/subject2template_warp.mif -noreorientation IN/fod_in_template_space_NOT_REORIENTED.mif


14. Segment FOD images to estimate fixels and their apparent fibre density (FD)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_AFD.rst


15. Reorient fixels
^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/reorient_fixels.rst

16. Assign subject fixels to template fixels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In step 8 & 9 we obtained spatial correspondence between subject and template. In step 14 we corrected the fixel orientations to ensure angular correspondence of the segmented peaks of subject and template. Here, for each fixel in the template fixel analysis mask, we identify the corresponding fixel in each voxel of the subject image and assign the FD value of the subject fixel to the corresponding fixel in template space. If no fixel exists in the subject that corresponds to the template fixel then it is assigned a value of zero. See `this paper <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`__ for more information. In the command below, you will note that the output fixel directory is the same for all subjects. This directory now stores data for all subjects at corresponding fixels, ready for input to :code:`fixelcfestats` in step 20 below::

    foreach * : fixelcorrespondence IN/fixel_in_template_space/fd.mif ../template/fixel_mask ../template/fd PRE.mif

17. Compute fibre cross-section (FC) metric
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_FC.rst

18. Compute a combined measure of fibre density and cross-section (FDC)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_FDC.rst

19. Perform whole-brain fibre tractography on the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/tractography.rst

20. Reduce biases in tractogram densities
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/sift.rst

21. Perform statistical analysis of FD, FC, and FDC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/statistics.rst

22. Visualise the results
^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/visualisation.rst








