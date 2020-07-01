Fibre density and cross-section - Multi-tissue CSD
==================================================

Introduction
------------

This tutorial explains how to perform fixel-based analysis of fibre density and cross-section [Raffelt2017]_ with fibre orientation distributions (FODs) computed using multi-tissue (3-tissue) CSD variants [Jeurissen2014]_ [Dhollander2016a]_. We note that high b-value (>2000s/mm2) data is recommended to aid the interpretation of apparent fibre density (AFD) being related to the intra-axonal space. See [Raffelt2012]_ for some details about AFD; though note that the interpretation can be altered for multi-tissue (3-tissue) CSD, depending on the context and tissues in the model.

All steps in this tutorial are written as if the commands are being **run on a cohort of images**, and make extensive use of the :ref:`for_each script to simplify batch processing <batch_processing>`. This tutorial also assumes that the imaging dataset is organised with one directory identifying each subject, and all files within identifying the image type (i.e. processing step outcome). For example::

    study/subjects/001_control/dwi.mif
    study/subjects/002_control/dwi.mif
    ...
    study/subjects/020_control/dwi.mif
    study/subjects/021_patient/dwi.mif
    ...
    study/subjects/040_patient/dwi.mif

.. NOTE:: All commands at the start of this tutorial are run **from the subjects path**. From the step where tractography is performed on the template onwards, we change directory **to the template path**.

For all MRtrix scripts and commands, additional information on the command usage and available command-line options can be found by invoking the command with the :code:`-help` option.


Pre-processsing steps
---------------------

1. Denoising and unringing
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/denoise_unring.rst

2. Motion and distortion correction
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/dwifslpreproc.rst

3. Bias field correction
^^^^^^^^^^^^^^^^^^^^^^^^
The multi-tissue FBA pipeline corrects for bias fields (and jointly performs global intensity normalisation) at the later :ref:`mtnormalise` step. The only incentive for running the (less robust and accurate) :ref:`dwibiascorrect` at this stage in the pipeline is *to improve brain mask estimation* (at the later :ref:`dwi2mask` step, in case severe bias fields are present in the data). However, cases have been reported where running :ref:`dwibiascorrect` at this stage resulted in *inferior* brain mask estimation later on. This is probably more likely in case bias fields are not as strongly present in the data. Whether :ref:`dwibiascorrect` is run at this stage or not, does not have any significant impact on the performance of :ref:`mtnormalise` later on.

If or when performing DWI bias field correction at this stage, it is achieved by first estimating the bias field from the DWI b=0 data, then applying the field to correct all DW volumes, which is done in a single step using the :code:`ants` algorithm within the :ref:`dwibiascorrect` script in *MRtrix3*. The script uses a bias field correction algorithm available in `ANTs <http://stnava.github.io/ANTs/>`_ (the N4 algorithm). *Don't* use the :code:`fsl` algorithm with this script in this fixel-based analysis pipeline. To perform bias field correction on DW images, run::

    for_each * : dwibiascorrect ants IN/dwi_denoised_unringed_preproc.mif IN/dwi_denoised_unringed_preproc_unbiased.mif


Fixel-based analysis steps
--------------------------

4. Computing (average) tissue response functions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
A robust and fully automated unsupervised method to obtain 3-tissue response functions representing single-fibre white matter, grey matter and CSF from the data itself, is the approach proposed in [Dhollander2016b]_ with the improvements of [Dhollander2019]_, which can be run by::

    for_each * : dwi2response dhollander IN/dwi_denoised_unringed_preproc_unbiased.mif IN/response_wm.txt IN/response_gm.txt IN/response_csf.txt

It is crucial for fixel-based analysis to only use a single *unique* set of the (three) response functions to perform (3-tissue) spherical deconvolution of all subjects: as the (3-tissue) spherical deconvolution results will be expressed in function of this set of response functions, they can (in an abstract way) be seen as the units of both the final apparent fibre density metric and the other compartments estimated in the model. One possible way to obtain a unique set of response functions, is to average the response functions obtained from all subjects for each tissue type::

    responsemean */response_wm.txt ../group_average_response_wm.txt
    responsemean */response_gm.txt ../group_average_response_gm.txt
    responsemean */response_csf.txt ../group_average_response_csf.txt

There is however no strict requirement for the final set of response functions to be the average of *all* subject response functions, for each tissue type (or indeed, it doesn't even have to be the average per se). In certain very specific cases, it may even be wise to leave out subjects (for this step) where the response functions could not reliably be obtained, or where pathology affected the brain globally.

5. Upsampling DW images
^^^^^^^^^^^^^^^^^^^^^^^
Upsampling DWI data *before* computing FODs increases anatomical contrast and improves downstream template building, registration, tractography and statistics. We recommend upsampling to an isotropic voxel size of 1.25 mm for human brains (if your original resolution is already higher, you can skip this step)::

    for_each * : mrgrid IN/dwi_denoised_unringed_preproc_unbiased.mif regrid -vox 1.25 IN/dwi_denoised_unringed_preproc_unbiased_upsampled.mif


6. Compute upsampled brain mask images
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Compute a whole brain mask from the upsampled DW images::

    for_each * : dwi2mask IN/dwi_denoised_unringed_preproc_unbiased_upsampled.mif IN/dwi_mask_upsampled.mif

.. WARNING:: It is absolutely **crucial** to check at this stage that *all* individual subject masks include *all* regions of the brain that are intended to be analysed. Fibre orientation distributions will *only* be computed within these masks; and at a later step (in template space) the analysis mask will be restricted to the *intersection* of all masks, so *any* individual subject mask which excludes a certain region, will result in this region being excluded from the entire analysis (unless a more advanced pipeline is followed; see :ref:`mitigating_brain_cropping`). Masks appearing too generous or otherwise including non-brain regions should generally not cause any concerns at this stage. Hence, if in doubt, it is advised to always err on the side of *inclusion* (of regions) at this stage.

.. NOTE:: The earlier :ref:`dwibiascorrect` step is not fundamentally important in the multi-tissue fixel-based analysis pipeline, as the later :ref:`mtnormalise` step performs more robustly (and if :ref:`dwibiascorrect` is included, :ref:`mtnormalise` will later on typically improve the result further). While performing the earlier :ref:`dwibiascorrect` step typically improves :ref:`dwi2mask` performance, cases have been observed where the opposite is true (typically if the data contains only weak bias fields). If required, experiment by either including or excluding :ref:`dwibiascorrect` in the pipeline in function of the best :ref:`dwi2mask` outcome and manually correct the masks if necessary (by *adding* regions which :ref:`dwi2mask` fails to include).

7. Fibre Orientation Distribution estimation (multi-tissue spherical deconvolution)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When performing fixel-based analysis, multi-tissue constrained spherical deconvolution should be performed using the unique set of (average) tissue response functions obtained before::

    for_each * : dwi2fod msmt_csd IN/dwi_denoised_unringed_preproc_unbiased_upsampled.mif ../group_average_response_wm.txt IN/wmfod.mif ../group_average_response_gm.txt IN/gm.mif  ../group_average_response_csf.txt IN/csf.mif -mask IN/dwi_mask_upsampled.mif


8. Joint bias field correction and intensity normalisation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To perform joint bias field correction and global intensity normalisation of the multi-tissue compartment parameters, use :ref:`mtnormalise`::

    for_each * : mtnormalise IN/wmfod.mif IN/wmfod_norm.mif IN/gm.mif IN/gm_norm.mif IN/csf.mif IN/csf_norm.mif -mask IN/dwi_mask_upsampled.mif

If multi-tissue CSD was performed with the same single set of (three) tissue response functions for all subjects, then the resulting output of :ref:`mtnormalise` makes the absolute amplitudes comparable between those subjects as well. Note that this step is **crucial** in the FBA pipeline, even if bias field correction was applied earlier using :ref:`dwibiascorrect`, since :ref:`dwibiascorrect` does *not* correct for *global* intensity differences between subjects. The performance of :ref:`mtnormalise` is not significantly impacted by either having run :ref:`dwibiascorrect` before or not. In case prior bias field correction was run in the pipeline, :ref:`mtnormalise` will further correct for residual intensity inhomogeneities.

.. WARNING:: :ref:`mtnormalise` results can be sensitive to masks that contain non-brain voxels. The underlying algorithm will attempt to drive the sum of tissue volumes to unity in such voxels - despite not containing brain tissue - which can result in erroneous bias field correction if the number of such voxels is large. For this reason we recommend using conservative (i.e. less spatially extended) masks for the :ref:`mtnormalise` step. Unlike step 6, where inclusion of all brain voxels was encouraged even at the expense of including some non-brain voxels, for bias field estimation exclusion of non-brain voxels is of greater priority than inclusion of all brain voxels.


9. Generate a study-specific unbiased FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/population_template.rst

Symbolic link all FOD images (and masks) into a single input folder. To use the entire population to build the template::

    for_each * : ln -sr IN/wmfod_norm.mif ../template/fod_input/PRE.mif
    for_each * : ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif

If you opt to create the template from a limited subset of (e.g. 30-40) subjects and your study has multiple groups, then you can aim for a similar number of subjects from each group to make the template more representative of the population as a whole. Assuming the subject directory labels can be used to identify members of each group, you could use::

    for_each `ls -d *patient | sort -R | tail -20` : ln -sr IN/wmfod_norm.mif ../template/fod_input/PRE.mif ";" ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif
    for_each `ls -d *control | sort -R | tail -20` : ln -sr IN/wmfod_norm.mif ../template/fod_input/PRE.mif ";" ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif

.. include:: common_fba_steps/population_template2.rst


10. Register all subject FOD images to the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Register the FOD image from each subject to the FOD template::

    for_each * : mrregister IN/wmfod_norm.mif -mask1 IN/dwi_mask_upsampled.mif ../template/wmfod_template.mif -nl_warp IN/subject2template_warp.mif IN/template2subject_warp.mif


11. Compute the template mask (intersection of all subject masks in template space)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/template_mask.rst


12. Compute a white matter template analysis fixel mask
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this step, we segment fixels from the FOD template. The result is the *fixel mask* that defines the fixels for which statistical analysis will later on be performed (and hence also which fixels' statistics can support others via the mechanism of connectivity-based fixel enhancement (CFE) [Raffelt2015]_)::

   fod2fixel -mask ../template/template_mask.mif -fmls_peak_value 0.06 ../template/wmfod_template.mif ../template/fixel_mask

.. NOTE:: Fixel images, which appear in the pipeline from this step onwards, are stored using the :ref:`fixel_format`, which stores all fixel data for a fixel image in a directory (i.e. a folder).

.. WARNING:: This step ultimately determines the fixel mask in which statistical analysis will be performed, and hence also which fixels' statistics can contribute to others via the CFE mechanism; so it may have a substantial impact on the final result. Essentially, it can be detrimental to the result if the threshold value specified via the :code:`-fmls_peak_value` is *too high* and hence *excludes* genuine white matter fixels. This risk is substantially higher in voxels containing crossing fibres (and higher the more fibres are crossing in a single voxel). Even though 0.06 has been observed to be a decent default value for 3-tissue CSD population templates, it is still **strongly advised** to visualise the output fixel mask using :ref:`mrview`. Do this by opening the :code:`index.mif` found in :code:`../template/fixel_mask` via the *fixel plot tool*. If, with respect to known or normal anatomy, fixels are missing (especially paying attention to crossing areas), regenerate the mask with a lower value supplied to the :code:`-fmls_peak_value` option (of course, avoid lowering it *too* much, as too many false or noisy fixels may be introduced). For an *adult human* brain template, and using an isotropic template voxel size of 1.25 mm, it is expected to have several *hundreds of thousands* of fixels in the fixel mask (you can check this by :code:`mrinfo -size ../template/fixel_mask/directions.mif`, and looking at the size of the image along the first dimension).

13. Warp FOD images to template space
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Note that here we warp FOD images into template space *without* FOD reorientation, as reorientation will be performed in a separate subsequent step (after fixel segmentation)::

    for_each * : mrtransform IN/wmfod_norm.mif -warp IN/subject2template_warp.mif -reorient_fod no IN/fod_in_template_space_NOT_REORIENTED.mif


14. Segment FOD images to estimate fixels and their apparent fibre density (FD)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_AFD.rst


15. Reorient fixels
^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/reorient_fixels.rst

16. Assign subject fixels to template fixels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/fixelcorrespondence.rst

17. Compute the fibre cross-section (FC) metric
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_FC.rst

18. Compute a combined measure of fibre density and cross-section (FDC)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_FDC.rst

19. Perform whole-brain fibre tractography on the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Statistical analysis using connectivity-based fixel enhancement (CFE) [Raffelt2015]_ exploits local connectivity information derived from probabilistic fibre tractography, which acts as a neighbourhood definition for threshold-free enhancement of locally clustered statistic values. To generate a whole-brain tractogram from the FOD template (note the remaining steps from here on are executed from the template directory)::

    cd ../template
    tckgen -angle 22.5 -maxlen 250 -minlen 10 -power 1.0 wmfod_template.mif -seed_image template_mask.mif -mask template_mask.mif -select 20000000 -cutoff 0.06 tracks_20_million.tck

.. WARNING:: The appropriate FOD amplitude cutoff for FOD template tractography can vary considerably between different datasets, as well as different versions of *MRtrix3* due to historical software bugs. While the value of 0.06 is suggested as a reasonable value for multi-tissue data, it may be beneficial to first generate a smaller number of streamlines (e.g. 100,000) using this value, and visually confirm that the generated streamlines exhibit an appropriate extent of propagation at the ends of white matter pathways, before committing to generation of the dense tractogram.

20. Reduce biases in tractogram densities
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/sift.rst

21. Generate fixel-fixel connectivity matrix
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/matrix.rst

22. Smooth fixel data using fixel-fixel connectivity
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/smooth.rst

23. Perform statistical analysis of FD, FC, and FDC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/statistics.rst

24. Visualise the results
^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/visualisation.rst


