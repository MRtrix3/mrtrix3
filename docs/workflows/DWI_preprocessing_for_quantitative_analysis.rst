DWI Pre-processing for Quantitative Analysis
============================================

Introduction
------------
This tutorial explains the required pre-processing steps for downstream applications that depend on FOD images for quantitative analysis (e.g. `Fixel-Based Analysis <http://userdocs.mrtrix.org/en/latest/workflows/fixel_based_analysis.html>`_ of Apparent Fibre Density, as well as `SIFT <http://userdocs.mrtrix.org/en/latest/workflows/sift.html>`_-based connectome analysis). 

Most DWI models derive quantitative measures by using the ratio of the DW signal to the b=0 signal within each voxel. This voxel-wise b=0 normalisation implicitly removes intensity variations due to T2-weighting and RF inhomogeneity. However, unless all compartments within white matter are modelled accurately (e.g. intra- and extra-axonal space, myelin, cerebral spinal fluid (CSF) and grey matter partial volumes), the proportion of one compartment in a voxel may influence another. For example, if CSF partial volume at the border of white matter and the ventricles is not taken into account, then a voxel-wise normalisation performed by dividing by the b=0 (which has a long T2 and appears brighter in CSF than white matter in the T2-weighted b=0 image), will artificially reduce the DW signal from the white matter intra-axonal (restricted) compartment, ultimately changing the derived quantiative measures. Multi-compartment diffusion MRI models aspire to model multiple compartments to mitigate these issues. However, in practice current models are limited by restrictions/assumptions placed on the different compartments, and all require multiple b-value acquisitions and therefore longer scan times. 

`Apparent Fibre Density (AFD) <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_ is a Fibre Orientation Distribution (FOD)-derived measure that was developed to enable fibre-specific quantitative analysis using single-shell HARDI data. AFD is based on the assumption that under certain conditions (high b-value of ~3000 s/mm2, typical clinically achievable diffusion pulse duration, typical axon diameters in the range 1-4µm), the DW signal arising from the extra-axonal space (including CSF) is fully attenuated, and the remaining DW signal is proportional to the intra-axonal volume. Since the spherical deconvolution used to compute FODs is a linear transformation, the integral of the (unnormalised) FOD is proportional to the DW signal summed over all gradient directions (and therefore the intra-axonal space). FOD amplitude along a specific direction (the AFD) is therefore proportional to the intra-axonal volume of axons aligned with this direction. In `recent work <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_, we have opted to compute the AFD per fibre population by computing the FOD integral within each FOD lobe (see `here <http://www.ncbi.nlm.nih.gov/pubmed/23238430>`_ for details). 

To enable robust quantitative comparisons of AFD across subjects (or AFD-derived quantities such as SIFT-filtered tractograms) there are three steps required:

#. **Bias field correction** to eliminate low frequency intensity inhomogeneities across the image.

#. **Global intensity normalisation** by normalising the median CSF or WM b=0 intensity across all subjects (see below for more details). This avoids the above noted issues with voxel-wise b=0 normalisation, such as CSF partial volume influencing restricted white matter DW signal (and therefore the AFD).   

#. **Use the same single fibre response function** in the spherical deconvolution for all subjects. This ensures differences in intra-axonal volume (and therefore DW signal) across subjects are detected as differences in the FOD amplitude (the AFD). See `the AFD paper <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_ for more details.


Pre-processsing steps
---------------------
Below is a list of recommended pre-processing steps for quantitative analysis using FOD images. Note that for all MRtrix scripts and commands, additional information on the command usage and available command-line options can be found by invoking the command with the :code:`-help` option. 


1. DWI denoising
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The effective SNR of diffusion data can be improved considerably by exploiting the redundancy in the data to reduce the effects of thermal noise. This functionality is provided in the command ``dwidenoise``::

  dwidenoise <input_dwi> <output_dwi>

Note that this denoising step *must* be performed prior to any other image pre-processing: any form of image interpolation (e.g. re-gridding images following motion correction) will invalidate the statistical properties of the image data that are exploited by ``dwidenoise``, and make the denoising process prone to errors. Therefore this process is applied as the very first step.


2. DWI general pre-processing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The :code:`dwipreproc` script is provided for performing general pre-processing of diffusion image data - this includes eddy current-induced distortion correction, motion correction, and (possibly) susceptibility-induced distortion correction. Commands for performing this pre-processing are not yet implemented in *MRtrix3*; the :code:`dwipreproc` script in its current form is in fact an interface to the relevant commands that are provided as part of the `FSL <http://fsl.fmrib.ox.ac.uk/>`_ package. Installation of FSL (including `eddy <http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/EDDY>`_) is therefore required to use this script, and citation of the relevant articles is also required (see the :ref:`dwipreproc` help page).

Usage of this script varies depending on the specific nature of the DWI acquisition with respect to EPI phase encoding - full details are available within the :ref:`dwipreproc` help file. Here, only a simple example is provided, where a single DWI series is acquired where all volumes have an anterior-posterior (A>>P) phase encoding direction::

  dwipreproc AP <input_dwi> <output_dwi> -rpe_none


3. Estimate a brain mask
^^^^^^^^^^^^^^^^^^^^^^^^^
A whole-brain mask is required as input to the subsequent bias field correction step. This can be computed with::

  dwi2mask <input_dwi> <output_mask>
  
  
4. Bias field correction
^^^^^^^^^^^^^^^^^^^^^^^^
DWI bias field correction is perfomed by first estimating a correction field from the DWI b=0 image, then applying the field to correct all DW volumes. This can be done in a single step using the :code:`dwibiascorrect` script in MRtrix. The script uses bias field correction algorthims available in `ANTS <http://stnava.github.io/ANTs/>`_ or `FSL <http://fsl.fmrib.ox.ac.uk/>`_. In our experience the `N4 algorithm <http://www.ncbi.nlm.nih.gov/pmc/articles/PMC3071855/>`_ in ANTS gives superiour results. To install N4 install the `ANTS <http://stnava.github.io/ANTs/>`_ package, then run perform bias field correction on DW images using::

    dwibiascorrect -ants -mask <input_brain_mask> <input_dwi> <output_corrected_dwi>
    
    
5. Global intensity normalisation across subjects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  
The ideal approach is to normalise the median CSF b=0 intensity across all subjects (on the assumption that the CSF T2 is unlikely to be affected by pathology). However, in practice it is difficult to obtain a robust partial-volume-free estimate of the CSF intensity due to the typical low resolution of DW images. For participants less than 50 years old (with reasonably small ventricles), it can be difficult to identify pure CSF voxels at 2-2.5mm resolutions. We therefore recommend performing a global intensity normalisation using the median white matter b=0 intensity. While the white matter b=0 intensity may be influenced by pathology-induced changes in T2, our assumption is that such changes will be local to the pathology and therefore have little influence on the median b=0 value. 

We have included the :code:`dwiintensitynorm` script in MRtrix to perform an automatic global normalisation using the median white matter b=0 value. The script input requires two folders: a folder containing all DW images in the study (in .mif format) and a folder containing the corresponding whole brain mask images (with the same filename prefix). The script runs by first computing diffusion tensor Fractional Anisotropy (FA) maps, registering these to a study-specific template, then thresholding the template FA map to obtain an approximate white matter mask. The mask is then transformed back into the space of each subject image and used in the :code:`dwinormalise` command to normalise the input DW images to have the same b=0 white matter median value. All intensity normalised data will be output in a single folder::

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

