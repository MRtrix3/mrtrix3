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

.. WARNING:: It is absolutely **crucial** to check at this stage that *all* individual subject masks include *all* regions of the brain that are intended to be analysed. Fibre orientation distributions will *only* be computed within these masks; and at a later step (in template space) the analysis mask will be restricted to the *intersection* of all masks, so *any* individual subject mask which excludes a certain region, will result in this region being excluded from the entire analysis. Masks appearing too generous or otherwise including non-brain regions should generally not cause any concerns at this stage. Hence, if in doubt, it is advised to always err on the side of *inclusion* (of regions) at this stage.

.. NOTE:: The earlier :ref:`dwibiascorrect` step is not fundamentally important in the multi-tissue fixel-based analysis pipeline, as the later :ref:`mtnormalise` step performs more robustly (and if :ref:`dwibiascorrect` is included, :ref:`mtnormalise` will later on typically improve the result further). While performing the earlier :ref:`dwibiascorrect` step typically improves :ref:`dwi2mask` performance, cases have been observed where the opposite is true (typically if the data contains only weak bias fields). If required, experiment by either including or excluding :ref:`dwibiascorrect` in the pipeline in function of the best :ref:`dwi2mask` outcome and manually correct the masks if necessary (by *adding* regions which :ref:`dwi2mask` fails to include).

7. Fibre Orientation Distribution estimation (multi-tissue spherical deconvolution)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When performing fixel-based analysis, multi-tissue constrained spherical deconvolution should be performed using the unique set of (average) tissue response functions obtained before::

    foreach * : dwi2fod msmt_csd IN/dwi_denoised_unringed_preproc_unbiased_upsampled.mif ../group_average_response_wm.txt IN/wmfod.mif ../group_average_response_gm.txt IN/gm.mif  ../group_average_response_csf.txt IN/csf.mif -mask IN/dwi_mask_upsampled.mif


8. Joint bias field correction and intensity normalisation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To perform joint bias field correction and global intensity normalisation of the multi-tissue compartment model results in the log-domain using :ref:`mtnormalise`::

    foreach * : mtnormalise IN/wmfod.mif IN/wmfod_norm.mif IN/gm.mif IN/gm_norm.mif IN/csf.mif IN/csf_norm.mif -mask IN/dwi_mask_upsampled.mif

If multi-tissue CSD was performed with the same single set of (three) tissue response functions for all subjects, then the resulting output of :ref:`mtnormalise` makes the amplitudes comparable between those subjects as well. Note that this step is **crucial** in the FBA pipeline, even if bias field correction was applied earlier using :ref:`dwibiascorrect`, since :ref:`dwibiascorrect` does *not* correct for *global* intensity differences between subjects. The performance of :ref:`mtnormalise` is not significantly impacted by either having run :ref:`dwibiascorrect` before or not. In case prior bias field correction was run in the pipeline, :ref:`mtnormalise` will further correct for residual intensity inhomogeneities.


9. Generate a study-specific unbiased FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/population_template.rst

Symbolic link all FOD images (and masks) into a single input folder. If you have fewer than 40 subjects in your study, you can use the entire population to build the template::

    foreach * : ln -sr IN/wmfod_norm.mif ../template/fod_input/PRE.mif
    foreach * : ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif

Alternatively, if you have more than 40 subjects you can randomly select a subset of the individuals. If your study has multiple groups, then you may want to aim for a similar number of subjects from each group to make the template more representative of the population as a whole. Assuming the subject directory labels can be used to identify members of each group, you could use::

    foreach `ls -d *patient | sort -R | tail -20` : ln -sr IN/wmfod_norm.mif ../template/fod_input/PRE.mif ";" ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif
    foreach `ls -d *control | sort -R | tail -20` : ln -sr IN/wmfod_norm.mif ../template/fod_input/PRE.mif ";" ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif

.. include:: common_fba_steps/population_template2.rst

10. Register all subject FOD images to the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Register the FOD image from each subject to the FOD template image. Note you can skip this step if you built your template from your entire population and saved the warps (see previous step)::

    foreach * : mrregister IN/wmfod_norm.mif -mask1 IN/dwi_mask_upsampled.mif ../template/wmfod_template.mif -nl_warp IN/subject2template_warp.mif IN/template2subject_warp.mif

11. Compute the template mask (intersection of all subject masks in template space)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/template_mask.rst


12. Compute a white matter template analysis fixel mask
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this step, we segment fixels from the FOD template. The result is the *fixel mask* that defines the fixels for which statistical analysis will later on be performed (and hence also which fixels' statistics can support others via the mechanism of `connectivity-based fixel enhancement (CFE) <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`__)::

   fod2fixel -mask ../template/template_mask.mif -fmls_peak_value 0.06 ../template/wmfod_template.mif ../template/fixel_mask

.. NOTE:: Fixel images, which appear in the pipeline from this step onwards, are stored using the :ref:`fixel_format`, which stores all fixel data for a fixel image in a directory (i.e. a folder).

.. WARNING:: This step ultimately determines the fixel mask in which stastical analysis will be performed, and hence also which fixels' statistics can contribute to others via the CFE mechanism; so it may have a substantial impact on the final result. Essentially, it can be detrimental to the result if the threshold value specified via the :code:`-fmls_peak_value` is *too high* and hence *excludes* genuine white matter fixels. This risk is higher in voxels containing crossing fibres (and higher the more fibres are crossing in a single voxel). Even though 0.06 has been observed to be a decent default value for 3-tissue CSD population templates, it is still **strongly advised** to visualise the output fixel mask using :ref:`mrview`. Do this by opening the :code:`index.mif` found in :code:`../template/fixel_mask` via the *fixel plot tool*. If, with respect to known or normal anatomy, fixels are missing (especially paying attention to crossing areas), regenerate the mask with a lower value supplied to the :code:`-fmls_peak_value` option (of course, avoid lowering it *too* much, as too many false or noisy fixels may be introduced). For an *adult human* brain template, and using an isotropic template voxel size of 1.3 mm, it is expected to have several *hundreds of thousands* of fixels in the fixel mask (you can check this by :code:`mrinfo -size ../template/fixel_mask/directions.mif`, and looking at the size of the image along the first dimension).

13. Warp FOD images to template space
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Note that here we warp FOD images into template space *without* FOD reorientation, as reorientation will be performed in a separate subsequent step (after fixel segmentation)::

    foreach * : mrtransform IN/wmfod_norm.mif -warp IN/subject2template_warp.mif -noreorientation IN/fod_in_template_space_NOT_REORIENTED.mif


14. Segment FOD images to estimate fixels and their apparent fibre density (FD)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_AFD.rst


15. Reorient fixels
^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/reorient_fixels.rst

16. Assign subject fixels to template fixels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/fixelcorrespondence.rst

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
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/statistics.rst

22. Visualise the results
^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/visualisation.rst


