Fibre density and cross-section - Single-tissue CSD
===================================================

Introduction
------------

This tutorial explains how to perform fixel-based analysis of fibre density and cross-section [Raffelt2017]_ using *single-tissue* spherical deconvolution. We note that high b-value (>2000s/mm2) data is recommended to aid the interpretation of apparent fibre density (AFD) being related to the intra-axonal space. See [Raffelt2012]_ for more details about *single-tissue* AFD.

All steps in this tutorial are written as if the commands are being **run on a cohort of images**, and make extensive use of the :ref:`foreach script to simplify batch processing <batch_processing>`. This tutorial also assumes that the imaging dataset is organised with one directory identifying each subject, and all files within identifying the image type (i.e. processing step outcome). For example::

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


3. Estimate a temporary brain mask
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Compute a brain mask::

    foreach * : dwi2mask IN/dwi_denoised_preproc.mif IN/dwi_temp_mask.mif
    

AFD-specific pre-processsing steps
----------------------------------

To enable robust quantitative comparisons of AFD across subjects three additional steps are required. Note these can potentially be skipped if analysing *certain* other DWI fixel-based measures related to fibre density (for example CHARMED).


4. Bias field correction
^^^^^^^^^^^^^^^^^^^^^^^^

Because we recommend a :ref:`global intensity normalisation <global-intensity-normalisation>`, bias field correction is required as a pre-processing step to eliminate low frequency intensity inhomogeneities across the image. DWI bias field correction is perfomed by first estimating the bias field from the DWI b=0 data, then applying the field to correct all DW volumes. This can be done in a single step using the :ref:`dwibiascorrect` script with the :code:`-ants` option in MRtrix. The script uses a bias field correction algorithm available in `ANTS <http://stnava.github.io/ANTs/>`_ (the N4 algorithm). *Don't* use the :code:`-fsl` option with this script in this fixel-based analysis pipeline. To perform bias field correction on DW images, run::

    foreach * : dwibiascorrect -ants IN/IN/dwi_denoised_unringed_preproc.mif IN/IN/dwi_denoised_unringed_preproc_unbiased.mif


5. Global intensity normalisation across subjects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As outlined :ref:`here <global-intensity-normalisation>`, a global intensity normalisation is required for AFD analysis. For a single-tissue pipeline, a possible approach is to use the :ref:`dwiintensitynorm` script. The script performs normalisation on all subjects within a study (using a group-wise registration), and therefore the input and output arguments are directories containing all study images. First create directories to store all the input and output images. From the subjects directory::

    mkdir -p ../dwiintensitynorm/dwi_input
    mkdir ../dwiintensitynorm/mask_input

You could copy all files into this directory, however symbolic linking them will save space::

    foreach * : ln -sr IN/dwi_denoised_unringed_preproc_unbiased.mif ../dwiintensitynorm/dwi_input/IN.mif
    foreach * : ln -sr IN/dwi_temp_mask.mif ../dwiintensitynorm/mask_input/IN.mif

Perform intensity normalisation::

    dwiintensitynorm ../dwiintensitynorm/dwi_input/ ../dwiintensitynorm/mask_input/ ../dwiintensitynorm/dwi_output/ ../dwiintensitynorm/fa_template.mif ../dwiintensitynorm/fa_template_wm_mask.mif

Link the output files back to the subject directories::

    foreach ../dwiintensitynorm/dwi_output/* : ln -sr IN PRE/dwi_denoised_unringed_preproc_unbiased_normalised.mif

The dwiintensitynorm script also outputs the study-specific FA template and white matter mask. **It is recommended that you check that the white matter mask is appropriate** (i.e. does not contain CSF or voxels external to the brain. It needs to be a rough WM mask). If you feel the white matter mask needs to be larger or smaller you can re-run :code:`dwiintensitynorm` with a different :code:`-fa_threshold` option. Note that if your input brain masks include CSF then this can cause spurious high FA values outside the brain which are then included in the template white matter mask.

Keeping the FA template image and white matter mask is also handy if additional subjects are added to the study at a later date. New subjects can be intensity normalised in a single step by :ref:`piping <unix_pipelines>` the following commands together. Run from the subjects directory::

    dwi2tensor new_subject/dwi_denoised_unringed_preproc_unbiased.mif -mask new_subject/dwi_temp_mask.mif - | tensor2metric - -fa - | mrregister -force ../dwiintensitynorm/fa_template.mif - -mask2 new_subject/dwi_temp_mask.mif -nl_scale 0.5,0.75,1.0 -nl_niter 5,5,15 -nl_warp - /tmp/dummy_file.mif | mrtransform ../dwiintensitynorm/fa_template_wm_mask.mif -template new_subject/dwi_denoised_unringed_preproc_unbiased.mif -warp - - | dwinormalise new_subject/dwi_denoised_unringed_preproc_unbiased.mif - ../dwiintensitynorm/dwi_output/new_subject.mif

.. NOTE:: The above command may also be useful if you wish to alter the mask and re-apply the intensity normalisation to all subjects in the study. For example, you may wish to edit the mask using the ROI tool in :code:`mrview` to *remove* white matter regions that you hypothesise are affected by the disease (e.g. removing the corticospinal tract in a study of motor neurone disease due to T2 hyperintensity). You also may wish to redefine the mask entirely, for example in an elderly population (with enlarged ventricles) it may be possible, or even preferable, to normalise using the median b=0 *CSF*. This could be performed by manually masking partial-volume-free CSF voxels, then running the above command with the CSF mask instead of the :code:`fa_template_wm_mask.mif`.

.. WARNING:: We strongly recommend you that you check the scale factors applied during intensity normalisation are not influenced by the variable of interest in your study. For example if one group contains global changes in white matter T2 then this may directly influence the intensity normalisation and therefore bias downstream AFD analysis. To check this, you can perform an equivalence test to ensure mean scale factors are the same between groups. To output the scale factor applied for all subjects use :code:`mrinfo ../dwiintensitynorm/dwi_output/* -property dwi_norm_scale_factor`.

Fixel-based analysis steps
--------------------------

6. Computing an (average) white matter response function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
A robust and fully automated (unsupervised) method to obtain single-shell response functions representing single-fibre white matter from your data, is the approach proposed in [Tournier2013]_, which can be run by::

    foreach * : dwi2response tournier IN/dwi_denoised_unringed_preproc_unbiased_normalised.mif IN/response.txt

It is crucial for fixel-based analysis to only use a single *unique* response function to perform spherical deconvolution of all subjects: as all resulting fibre orientation distributions will be expressed in function of it, it can (in an abstract way) be seen as the unit of the final apparent fibre density metric. A possible way to obtain a unique response function, is to average the response functions obtained from all subjects::

    average_response */response.txt ../group_average_response.txt

There is however no strict requirement for the (one) final response function to be the average of *all* subject response functions. In certain very specific cases, it may even be wise to leave out subjects (for this step) where a response function could not reliably be obtained, or where pathology affected the brain globally.

7. Upsampling DW images
^^^^^^^^^^^^^^^^^^^^^^^
Upsampling DWI data *before* computing FODs can increase anatomical contrast and improve downstream template building, registration, tractography and statistics. We recommend upsampling to an isotropic voxel size of 1.3 mm for human brains (if your original resolution is already higher, you can skip this step)::

    foreach * : mrresize IN/dwi_denoised_unringed_preproc_unbiased_normalised.mif -vox 1.3 IN/dwi_denoised_unringed_preproc_unbiased_normalised_upsampled.mif
    
8. Compute upsampled brain mask images
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Compute a whole brain mask from the upsampled DW images::

    foreach * : dwi2mask IN/dwi_denoised_unringed_preproc_unbiased_normalised_upsampled.mif IN/dwi_mask_upsampled.mif

.. WARNING:: It is absolutely **crucial** to check at this stage that *all* individual subject masks include *all* regions of the brain that are intended to be analysed. Fibre orientation distributions will *only* be computed within these masks; and at a later step (in template space) the analysis mask will be restricted to the *intersection* of all masks, so *any* individual subject mask which excludes a certain region, will result in this region being excluded from the entire analysis. Masks appearing too generous or otherwise including non-brain regions should generally not cause any concerns at this stage. Hence, if in doubt, it is advised to always err on the side of *inclusion* (of regions) at this stage. Manually correct the masks if necessary.

9. Fibre Orientation Distribution estimation (spherical deconvolution)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When performing fixel-based analysis, constrained spherical deconvolution (CSD) should be performed using the unique (average) white matter response function obtained before. Note that :code:`dwi2fod csd` can be used, however here we use :code:`dwi2fod msmt_csd` (even with single shell data) to benefit from the hard non-negativity constraint, which has been observed to lead to more robust outcomes::

    foreach * : dwiextract IN/dwi_denoised_unringed_preproc_unbiased_normalised_upsampled.mif - \| dwi2fod msmt_csd - ../group_average_response.txt IN/wmfod.mif -mask IN/dwi_mask_upsampled.mif

10. Generate a study-specific unbiased FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/population_template.rst

Symbolic link all FOD images (and masks) into a single input folder. To use the entire population to build the template::

    foreach * : ln -sr IN/wmfod.mif ../template/fod_input/PRE.mif
    foreach * : ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif

If you opt to create the template from a limited subset of (e.g. 30-40) subjects and your study has multiple groups, then you can aim for a similar number of subjects from each group to make the template more representative of the population as a whole. Assuming the subject directory labels can be used to identify members of each group, you could use::

    foreach `ls -d *patient | sort -R | tail -20` : ln -sr IN/wmfod.mif ../template/fod_input/PRE.mif ";" ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif
    foreach `ls -d *control | sort -R | tail -20` : ln -sr IN/wmfod.mif ../template/fod_input/PRE.mif ";" ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif

.. include:: common_fba_steps/population_template2.rst


11. Register all subject FOD images to the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Register the FOD image from each subject to the FOD template::

    foreach * : mrregister IN/wmfod.mif -mask1 IN/dwi_mask_upsampled.mif ../template/wmfod_template.mif -nl_warp IN/subject2template_warp.mif IN/template2subject_warp.mif


12. Compute the template mask (intersection of all subject masks in template space)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/template_mask.rst
    
    
13. Compute a white matter template analysis fixel mask
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In this step, we segment fixels from the FOD template. The result is the *fixel mask* that defines the fixels for which statistical analysis will later on be performed (and hence also which fixels' statistics can support others via the mechanism of connectivity-based fixel enhancement (CFE) [Raffelt2015]_)::

   fod2fixel -mask ../template/template_mask.mif -fmls_peak_value 0.10 ../template/wmfod_template.mif ../template/fixel_mask

.. NOTE:: Fixel images, which appear in the pipeline from this step onwards, are stored using the :ref:`fixel_format`, which stores all fixel data for a fixel image in a directory (i.e. a folder).

.. WARNING:: This step ultimately determines the fixel mask in which statistical analysis will be performed, and hence also which fixels' statistics can contribute to others via the CFE mechanism; so it may have a substantial impact on the final result. Essentially, it can be detrimental to the result if the threshold value specified via the :code:`-fmls_peak_value` is *too high* and hence *excludes* genuine white matter fixels. This risk is substantially higher in voxels containing crossing fibres (and higher the more fibres are crossing in a single voxel). Even though 0.1 has been observed to be a reasonable value for single-tissue CSD population templates, it is still **strongly advised** to visualise the output fixel mask using :ref:`mrview`. Do this by opening the :code:`index.mif` found in :code:`../template/fixel_mask` via the *fixel plot tool*. If, with respect to known or normal anatomy, fixels are missing (especially paying attention to crossing areas), regenerate the mask with a lower value supplied to the :code:`-fmls_peak_value` option. At the same time, try to avoid introducing too many noisy fixels, especially in gray matter areas. For single-tissue CSD, this balance is however not always easy to find. For an *adult human* brain template, and using an isotropic template voxel size of 1.3 mm, it is expected to have several *hundreds of thousands* of fixels in the fixel mask (you can check this by :code:`mrinfo -size ../template/fixel_mask/directions.mif`, and looking at the size of the image along the first dimension).

14. Warp FOD images to template space
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Note that here we warp FOD images into template space *without* FOD reorientation, as reorientation will be performed in a separate subsequent step (after fixel segmentation)::

    foreach * : mrtransform IN/wmfod.mif -warp IN/subject2template_warp.mif -noreorientation IN/fod_in_template_space_NOT_REORIENTED.mif


15. Segment FOD images to estimate fixels and their apparent fibre density (FD)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_AFD.rst

    
16. Reorient fixels
^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/reorient_fixels.rst
    
17. Assign subject fixels to template fixels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/fixelcorrespondence.rst

18. Compute the fibre cross-section (FC) metric
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_FC.rst

19. Compute a combined measure of fibre density and cross-section (FDC)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/compute_FDC.rst
    
20. Perform whole-brain fibre tractography on the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Statistical analysis using connectivity-based fixel enhancement (CFE) [Raffelt2015]_ exploits local connectivity information derived from probabilistic fibre tractography, which acts as a neighbourhood definition for threshold-free enhancement of locally clustered statistic values. To generate a whole-brain tractogram from the FOD template (note the remaining steps from here on are executed from the template directory)::

    cd ../template
    tckgen -angle 22.5 -maxlen 250 -minlen 10 -power 1.0 wmfod_template.mif -seed_image template_mask.mif -mask template_mask.mif -select 20000000 -cutoff 0.10 tracks_20_million.tck
    
21. Reduce biases in tractogram densities
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/sift.rst
    
22. Perform statistical analysis of FD, FC, and FDC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/statistics.rst

23. Visualise the results
^^^^^^^^^^^^^^^^^^^^^^^^^

.. include:: common_fba_steps/visualisation.rst


