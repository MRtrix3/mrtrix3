Fibre Density and Cross-section - Single Shell DWI
==================================================

Introduction
-------------

This tutorial explains how to perform `fixel-based analysis of fibre density and cross-section <https://www.ncbi.nlm.nih.gov/pubmed/27639350>`_ using single-shell data. While the focus here is on the analysis of `Apparent Fibre Density (AFD) <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_ derived from FODs, other fixel-based measures related to fibre density can also be analysed with a few minor modifications to these steps (as outlined below). We note that high b-value (>2000s/mm2) data is recommended to aid the interpretation of AFD being related to the intra-axonal space. See the `original paper <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_ for more details.


All steps in this tutorial have written as if the commands are being **run on a cohort of images**, and make extensive use of the :ref:`foreach script to simplify batch processing <batch_processing>`. This tutorial also assumes that the imaging dataset is organised with one directory identifying the subject, and all files within identifying the image type. For example::

    study/subjects/001_patient/dwi.mif
    study/subjects/001_patient/t1.mif
    study/subjects/002_control/dwi.mif
    study/subjects/002_control/t1.mif

.. NOTE:: All commands in this tutorial are run **from the subjects path** up until step 18, where we change directory to the template path

For all MRtrix scripts and commands, additional information on the command usage and available command-line options can be found by invoking the command with the :code:`-help` option. Please post any questions or issues on the `MRtrix community forum <http://community.mrtrix.org/>`_.


Pre-processsing steps
---------------------

.. toctree::
   :maxdepth: 2

   common_fba_steps/dwidenoise
   common_fba_steps/dwipreproc



3. Estimate a brain mask
^^^^^^^^^^^^^^^^^^^^^^^^^
Compute a bran mask::

    foreach * : dwi2mask IN/dwi_denoised_preproc.mif IN/dwi_mask.mif


AFD-specific pre-processsing steps
--------------------------------

To enable robust quantitative comparisons of AFD across subjects three additional steps are required. Note these can be skipped if analysing other DWI fixel-based measures related to fibre density (for example CHARMED).


4. Bias field correction
^^^^^^^^^^^^^^^^^^^^^^^^
Because we recommend a :ref:`global intensity normalisation <global-intensity-normalisation>`, bias field correction is required as a pre-processing step to eliminate low frequency intensity inhomogeneities across the image. DWI bias field correction is perfomed by first estimating a correction field from the DWI b=0 image, then applying the field to correct all DW volumes. This can be done in a single step using the :ref:`dwibiascorrect` script in MRtrix. The script uses bias field correction algorthims available in `ANTS <http://stnava.github.io/ANTs/>`_ or `FSL <http://fsl.fmrib.ox.ac.uk/>`_. In our experience the `N4 algorithm <http://www.ncbi.nlm.nih.gov/pmc/articles/PMC3071855/>`_ in ANTS gives superiour results. To install N4 install the `ANTS <http://stnava.github.io/ANTs/>`_ package, then run perform bias field correction on DW images using::

    foreach * : dwibiascorrect -ants -mask IN/dwi_mask.mif IN/dwi_denoised_preproc.mif IN/dwi_denoised_preproc_bias.mif


5. Global intensity normalisation across subjects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As outlined :ref:`here <global-intensity-normalisation>`, a global intensity normalisation is required for AFD analysis. For single-shell data this can be achieved using the :ref:`dwiintensitynorm` script. The script performs normalisation on all subjects within a study (using a group-wise registration), and therefore the input and output arguments are directories containing all study images. First create directories to store all the input and output images. From the subjects directory::

    mkdir -p ../dwiintensitynorm/dwi_input
    mkdir ../dwiintensitynorm/mask_input

You could copy all files into this directory, however symbolic linking them will save space::

    foreach * : ln -sr IN/dwi_denoised_preproc_bias.mif ../dwiintensitynorm/dwi_input/IN.mif
    foreach * : ln -sr IN/dwi_mask.mif ../dwiintensitynorm/mask_input/IN.mif

Perform intensity normalisation::

    dwiintensitynorm ../dwiintensitynorm/dwi_input/ ../dwiintensitynorm/mask_input/ ../dwiintensitynorm/dwi_output/ ../dwiintensitynorm/fa_template.mif ../dwiintensitynorm/fa_template_wm_mask.mif

Link the output files back to the subject directories::

    foreach ../dwiintensitynorm/dwi_output/* : ln -sr IN PRE/dwi_denoised_preproc_bias_norm.mif

The dwiintensitynorm script also outputs the study-specific FA template and white matter mask. **It is recommended that you check that the white matter mask is appropriate** (i.e. does not contain CSF or voxels external to the brain. Note it only needs to be a rough WM mask). If you feel the white matter mask needs to be larger or smaller you can re-run :code:`dwiintensitynorm` with a different :code:`-fa_threshold` option. Note that if your input brain masks include CSF then this can cause spurious high FA values outside the brain which will may be included in the template white matter mask.

Keeping the FA template image and white matter mask is also handy if additional subjects are added to the study at a later date. New subjects can be intensity normalised in a single step by :ref:`piping <unix-pipelines>` the following commands together. Run from the subjects directory::

    dwi2tensor new_subject/dwi_denoised_preproc_bias.mif -mask new_subject/dwi_mask.mif - | tensor2metric - -fa - | mrregister -force ../dwiintensitynorm/fa_template.mif - -mask2 new_subject/dwi_mask.mif -nl_scale 0.5,0.75,1.0 -nl_niter 5,5,15 -nl_warp - /tmp/dummy_file.mif | mrtransform ../dwiintensitynorm/fa_template_wm_mask.mif -template new_subject/dwi_denoised_preproc_bias.mif -warp - - | dwinormalise new_subject/dwi_denoised_preproc_bias.mif - ../dwiintensitynorm/dwi_output/new_subject.mif

.. NOTE:: The above command may also be useful if you wish to alter the mask then re-apply the intensity normalisation to all subjects in the study. For example you may wish to edit the mask using the ROI tool in :code:`mrview` to remove white matter regions that you hypothesise are affected by the disease (e.g. removing the corticospinal tract in a study of motor neurone disease due to T2 hyperintensity). You also may wish to redefine the mask completely, for example in an elderly population (with larger ventricles) it may be appropriate to intensity normalise using the median b=0 CSF. This could be performed by manually masking partial-volume-free CSF voxels, then running the above command with the CSF mask instead of the <input_template_wm_mask>.

.. WARNING:: We also strongly recommend you that you check the scale factors applied during intensity normalisation are not influenced by the variable of interest in your study. For example if one group contains global changes in white matter T2 then this may directly influence the intensity normalisation and therefore bias downstream AFD analysis. To check this we recommend you perform an equivalence test to ensure mean scale factors are the same between groups. To output the scale factor applied for all subjects use :code:`mrinfo ../dwiintensitynorm/dwi_output/* -property dwi_norm_scale_factor`.

6. Computing a group average response function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
As described `here <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_, using the same response function when estimating FOD images for all subjects enables differences in the intra-axonal volume (and therefore DW signal) across subjects to be detected as differences in the FOD amplitude (the AFD). To ensure the response function is representative of your study population, a group average response function can be computed by first estimating a response function per subject, then averaging with the script::

    foreach * : dwi2response tournier IN/dwi_denoised_preproc_bias_norm.mif IN/response.txt
    average_response */response.txt ../group_average_response.txt


Fixel-based analysis steps
---------------------------

7. Upsampling DW images
^^^^^^^^^^^^^^^^^^^^^^^
Upsampling DWI data before computing FODs can `increase anatomical contrast <http://www.sciencedirect.com/science/article/pii/S1053811914007472>`_ and improve downstream spatial normalisation and statistics. We recommend upsampling by a factor of two using bspline interpolation::

    foreach * : mrresize IN/dwi_denoised_preproc_bias_norm.mif -scale 2.0 IN/dwi_denoised_preproc_bias_norm_upsampled.mif
    
8. Compute upsampled brain mask images
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Compute a whole brain mask from the upsampled DW images::
    
    foreach * : dwi2mask IN/dwi_denoised_preproc_bias_norm_upsampled.mif IN/dwi_mask_upsampled.mif

Depending on your data, you may find that upsampling the low-resolution masks from step 3 gives superiour masks (with less holes). This can be performed using::

    foreach * : mrresize IN/dwi_mask.mif -scale 2.0 -inter nearest IN/dwi_mask_upsampled.mif

9. Fibre Orientation Distribution estimation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
When performing analysis of AFD, Constrained Spherical Deconvolution (CSD) should be performed using the group average response function computed at step . If not using AFD in the fixel-based analysis (and therefore you have skipped steps 4-6), however you still want to compute FODs for image registration, then you can use a subject-specific response function. Note that :code:`dwi2fod csd` can be used, however here we use :code:`dwi2fod msmt_csd` (even with single shell data) to benefit from the hard non-negativity constraint::

    foreach * : dwiextract IN/dwi_denoised_preproc_bias_norm_upsampled.mif - \| dwi2fod msmt_csd - ../group_average_response.txt IN/fod.mif -mask IN/dwi_mask_upsampled.mif

10. Generate a study-specific unbiased FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Population template creation is the most time consuming step in a fixel-based analysis. If you have a large number of subjects in your study, we recommend building the template from a subset of 20-40 individuals. Subjects should be chosen to ensure the generated template is representative of your population (i.e. equal number of patients and controls). To build a template, place all FOD images in a single folder. We also recommend placing a set of corresponding mask images (with the same prefix as the FOD images) in another folder. Using masks can speed up registration significantly::

    mkdir -p ../template/fod_input
    mkdir -p ../template/mask_input

Symbolic link all FOD images (and masks) into a single input folder. If you have fewer than 40 subjects in your study, you can use the entire population to build the template::

    foreach * : ln -sr IN/fod.mif ../template/fod_input/PRE.mif
    foreach * : ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif

Alternatively, if you have more than 40 subjects you can randomly select a subset of the individuals. If your study has multiple groups, then ideally you want to select the same number of subjects from each group to ensure the template is un-biased. Assuming the subject directory labels can be used to identify members of each group, you could use::

    foreach `ls -d *patient | sort -R | tail -20` : ln -sr IN/fod.mif ../template/fod_input/PRE.mif ";" ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif
    foreach `ls *control | sort -R | tail -20` : ln -sr IN/fod.mif ../template/fod_input/PRE.mif ";" ln -sr IN/dwi_mask_upsampled.mif ../template/mask_input/PRE.mif

Run the template building script as follows::

    population_template ../template/fod_input -mask_dir ../template/mask_input ../template/fod_template.mif

**If you are building a template from your entire study population**, run the population_template script use the :code:`-warp_dir warps` option to output a directory containing all subject warps to the template. Saving the warps here will enable you to skip the next step. Note that the warps used (and therefore output) from the population_template script are 5D images containing both forward and reverse warps (see :ref:`mrregister`for more info). To convert this warp format to a more conventional 4D deformation field format ready for the subsequent steps, run::

    foreach ../template/warps/* : warpconvert -type warpfull2deformation -template ../template/fod_template.mif IN PRE/subject2template_warp.mif

11. Register all subject FOD images to the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Register the FOD image from all subjects to the FOD template image::

    foreach * : mrregister IN/fod.mif -mask1 IN/dwi_mask_upsampled.mif ../template/fod_template.mif -nl_warp IN/subject2template_warp.mif IN/template2subject_warp.mif


12. Compute the intersection of all subject masks in template space
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Different subjects will have subtly different brain coverage. To ensure subsequent analysis is performed in voxels that contain data from all subjects, we warp all subject masks into template space and compute the mask intersection. For each subject::
    
    foreach * : mrtransform IN/dwi_mask_upsampled.mif -warp IN/subject2template_warp.mif -interp nearest IN/dwi_mask_in_template_space.mif

Compute the intersection of all warped masks::

    mrmath */dwi_mask_in_template_space.mif min ../template/mask_intersection.mif
    
    
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
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Note that here we warp FOD images into template space *without* FOD reorientation. Reorientation will be performed in a separate subsequent step::

    foreach * : mrtransform IN/fod.mif -warp IN/subject2template_warp.mif -noreorientation IN/fod_in_template_space.mif

15. Segment FOD images to estimate fixels and their fibre density (FD)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Here we segment each FOD lobe to identify the number and orientation of fixels in each voxel. The output also contains the apparent fibre density (AFD) value per fixel estimated as the FOD lobe integral (see `here <http://www.sciencedirect.com/science/article/pii/S1053811912011615>`_ for details on FOD segmentation). Note that in the following steps we will use a more generic shortened acronym - Fibre Density (FD) instead of AFD, since the following steps can also apply for other measures of fibre density (see the note below). The terminology is also consistent with our `recent work <https://www.ncbi.nlm.nih.gov/pubmed/27639350>`_::

    foreach * : fod2fixel IN/fod_in_template_space.mif -mask ../template/voxel_mask.mif IN/fixel_in_template_space -afd fd.mif
    
.. NOTE:: If you would like to perform fixel-based analysis of metrics derived from other diffusion MRI models (e.g. CHARMED), replace steps 14 & 15. For example, in step 14 you can warp preprocessed DW images (also without any reorientation). In step 15 you could then estimate your DWI model of choice, and output the FD related measure to the :ref:`fixel_format`, ready for the subsequent fixel reorientation step.
    
    
16. Reorient fixel orientations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Here we reorient the direction of all fixels based on the Jacobian matrix (local affine transformation) at each voxel in the warp. Note that in-place fixel reorientation can be performed by specifing the output fixel folder to be the same as the input, and using the :code:`-force` option::

    foreach * : fixelreorient IN/fixel_in_template_space IN/subject2template_warp.mif IN/fixel_in_template_space --force
    
17. Assign subject fixels to template fixels
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In step 10 & 11 we obtained spatial correspondence between subject and template. In step 16 we corrected the fixel orientations to ensure angular correspondence of the segmented peaks of subject and template. Here, for each fixel in the template fixel analysis mask, we identify the corresponding fixel in each voxel of the subject image and assign the FD value of the subject fixel to the corresponding fixel in template space. If no fixel exists in the subject that corresponds to the template fixel then it is assigned a value of zero. See `this paper <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_ for more information. In the command below, you will note that the output fixel directory is the same for all subjects. This directory now stores data for all subjects at corresponding fixels, ready for input to :code:`fixelcfestats` in step 22 below::

    foreach * : fixelcorrespondence IN/fixel_in_template_space/fd.mif ../template/fixel_template ../template/fd PRE.mif
    
18. Compute fibre cross-section (FC) metric
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Apparent fibre density, and other related measures that are influenced by the quantity of restricted water, only permit the investigation of group differences in the number of axons that manifest as a change to *within-voxel* density. However, depending on the disease type and stage, changes to the number of axons may also manifest as macroscopic differences in brain morphology. This step computes a fixel-based metric related to morphological differences in fibre cross-section, where information is derived entirely from the warps generated during registration (see `this paper <https://www.ncbi.nlm.nih.gov/pubmed/27639350>`_ for more information)::

    foreach * : warp2metric IN/subject2template_warp.mif -fc ../template/fixel_template ../template/fc IN.mif

    
Note that the FC files will be used in the next step. However, for group statistical analysis of FC we recommend taking the log(FC) to ensure data are centred about zero and normally distributed. We could place all the log(FC) fixel data files in the same fixel directory as the FC files (as long as they are named differently. However to keep things tidy, create a separate fixel directory to store the log(FC) data and copy the fixel index and directions file across::

    mkdir ../template/log_fc
    cp ../template/fc/index.mif ../template/log_fc
    cp ../template/fc/directions.mif ../template/log_fc
    foreach * : mrcalc ../template/fc/IN.mif -log ../template/log_fc/IN.mif

19. Compute a combined measure of fibre density and cross-section (FDC)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
To account for changes to both within-voxel fibre density and macroscopic atrophy, fibre density and fibre cross-section must be combined (a measure we call fibre density & cross-section, FDC). This enables a more complete picture of group differences in white matter. Note that as discussed in `this paper <https://www.ncbi.nlm.nih.gov/pubmed/27639350>`_, group differences in FD or FC alone must be interpreted with care in crossing-fibre regions. However group differences in FDC are more directly interpretable. To generate the combined measure we 'modulate' the FD by FC::

    mkdir ../template/fdc
    cp ../template/fc/index.mif ../template/fdc
    cp ../template/fc/directions.mif ../template/fdc
    foreach * : mrcalc ../template/fd/IN.mif ../template/fc/IN.mif -mult ../template/fdc/IN.mif
    
20. Perform whole-brain fibre tractography on the FOD template
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Statistical analysis using `connectivity-based fixel enhancement <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_ exploits connectivity information derived from probabilistic fibre tractography. To generate a whole-brain tractogram from the FOD template. Note the remaining steps from here on are executed from the template directory::

    cd ../template
    tckgen -angle 22.5 -maxlen 250 -minlen 10 -power 1.0 fod_template.mif -seed_image voxel_mask.mif -mask voxel_mask.mif -number 20000000 tracks_20_million.tck
    
21. Reduce biases in tractogram densities
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Perform SIFT to reduce tractography biases in the whole-brain tractogram::

    tcksift tracks_20_million.tck fod_template.mif tracks_2_million_sift.tck -term_number 2000000
    
22. Perform statistical analysis of FD, FC, and FDC
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 Statistical analysis is performed using `connectivity-based fixel enhancement <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_, with a separate analysis for FD, FC, and FDC as follows::
 
     fixelcfestats fd files.txt design_matrix.txt contrast_matrix.txt input_tracks_2_million_sift.tck stats_fd
     fixelcfestats log_fc files.txt design_matrix.txt contrast_matrix.txt input_tracks_2_million_sift.tck stats_log_fc
     fixelcfestats fdc files.txt design_matrix.txt contrast_matrix.txt input_tracks_2_million_sift.tck stats_fdc

Where the input files.txt is a text file containing the filename of each file (i.e. not the full path) to be analysed inside the input fixel directory, each filename on a separate line. The line ordering should correspond to the lines in the design_matrix.txt. Note that for correlation analysis, a column of 1's will not be automatically included (as per FSL randomise). Note that fixelcfestats currently only accepts a single contrast. However if the opposite (negative) contrast is also required (i.e. a two-tailed test), then use the :code:`-neg` option. Several output files will generated all starting with the supplied prefix.

.. NOTE:: We recommend having no more than 500,000 fixels in the analysis_fixel_mask (you can check this by :code:`mrinfo -size ../template/fixel_template/mask.mif`, and looking at the size of the image along the 1st dimension), otherwise :ref:`fixelcfestats` will run out of RAM. A mask with 500,000 fixels will require a PC with 128GB of RAM for the statistical analysis step. To reduce RAM requirements, you could reduce the number of fixels by not upsamplng your data at step 7 (or upsample it less), or apply a higher threshold when computing the fixel analysis mask (at the risk of removing WM regions from your analysis).


23. Visualise the results
^^^^^^^^^^^^^^^^^^^^^^^^^
To view the results load the population FOD template image in :code:`mrview`, and overlay the fixel images using the vector plot tool. Note that p-value images are saved as 1-p-value. Therefore to visualise all p-values < 0.05, threshold the fixels using the vector plot tool at 0.95.








