DWI Pre-processing for Quantitative Analysis using FODs
=======================================================

Introduction
------------
This tutorial explains the required pre-processing steps for downstream applications that depend on FOD images for quantitative analysis (e.g. `Fixel-Based Analysis <http://userdocs.mrtrix.org/en/latest/workflows/fixel_based_analysis.html>`_ of Apparent Fibre Density, as well as `SIFT <http://userdocs.mrtrix.org/en/latest/workflows/sift.html>`_-based connectome analysis). 

Most DWI models derive quantitative measures by using the b=0 image to normalise the DW signal within each voxel. A voxel-wise b=0 normalisation implicitly removes intensity variations due to T2-weighting and RF inhomogeneity. However, unless all compartments within white matter are modelled accurately (e.g. intra- and extra-axonal space, cerebral spinal fluid (CSF) and grey matter partial volumes), the proportion of one compartment in a voxel may influence another. For example, if CSF partial volume at the border of white matter and the ventricles is not accounted, then a voxel-wise normalisation peformed by dividing by the b=0 (which has a long T2 and appears brighter in CSF than white matter in the T2-weighted b=0 image), will artificially reduce the DW signal from the white matter intra-axonal (restricted) compartment, ultimately changing the derived quantiative measures. Multi-compartment diffusion MRI models aspire to model multiple compartments to mitigate these issues, however in practise current models are limited by restrictions/assumptions placed on different compartments, and all require multiple b-value acquisitions and therefore longer scan times. 

`Apparent Fibre Density (AFD) <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_ is a Fibre Orientation Distribution (FOD)-derived measure that was developed to enable fibre-specific quantitative analysis using single shell HARDI data. AFD is based on the assumption that under certain conditions (high b-value, typical diffusion pulse duration, typical axon diameters 1-4µm), the DW signal arrising from the extra-axonal space (including CSF) is fully attenuated, and the remaining DW signal is proportional to the intra-axonal volume. Since the spherical deconvolution used to compute FODs is a linear transformation, the integral of the (unnormalised) FOD is proportional to the DW signal summed over all gradient directions (and therefore the intra-axonal space). FOD amplitude along a specific direction (the AFD) is therefore proportional to the intra-axonal volume of axons aligned with the direction. In `recent work <http://www.ncbi.nlm.nih.gov/pubmed/26004503>`_, we have opted to compute the AFD per fibre population by computing the FOD integral within each FOD lobe (see `here <http://www.ncbi.nlm.nih.gov/pubmed/23238430>`_ for details). 

To enable robust quantitative comparisons of AFD across subjects (or AFD-derived quantities such as SIFT-filtered tractograms) there are three steps required:

#. **Global intensity normalisation** of the median white matter b=0 value across all subjects. This avoids the above noted issues with voxel-wise b=0 normalisation, such as CSF partial volume influencing restricted white matter DW signal (and therefore the AFD).  

#. **Bias field correction** to eliminate intensity inhomogeniety

#. **Use the same single fibre response function** in the spherical deconvolution for all subjects. This ensures differences in intra-axonal volume (and therefore DW signal) across subjects are detected as differences in the FOD amplitude (the AFD). See `the AFD paper <http://www.ncbi.nlm.nih.gov/pubmed/22036682>`_ for more details.


Pre-processsing steps
---------------------
Below is a list of recommended pre-processing steps for quantitative analysis using FOD images. Note that for all MRtrix scripts and commands, additional information on the command usage and available command-line options can be found by invoking the command with the -help option. 


1. Susceptibility-induced distortion correction
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
If you have access to reversed phase-encode spin-echo image data, this can be used to correct the susceptibility-induced geometric distortions present in the diffusion images. Software for EPI distortion correction is not yet implemented in MRtrix, however we do provide a script for interfacing with `Topup <http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/TOPUP>`_, part of the FSL package. Note that dwipreproc will also run Eddy (see the next step).
::

  dwipreproc <Input DWI series> <Output corrected DWI series> <Phase encode dir>

For more details, see the header of the scripts/dwipreproc file. In particular, it is necessary to manually specify what type of reversed phase-encoding acquisition has taken place (if any), and provide the relevant input images.

2. Eddy current induced distortion correction and motion correction 
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
The :code:`dwipreproc` script also interfaces with FSL's `Eddy <http://www.ncbi.nlm.nih.gov/pubmed/26481672>`_ for correcting Eddy current-induced distortions and subject motion. Note that if you have reverse phase-encode images for EPI correction, the :code:`dwipreproc` script will simultaneously run EDDY to ensure only a single interpolation is performed. If you don't have reverse phase encode data, then Eddy can be run without topup using::

  dwipreproc <Input DWI series> <Output corrected DWI series>


3. Estimate a brain mask estimation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
A whole-brain mask is required as input to the subsequent bias field correction step. This can be computed with::

  dwi2mask <Input DWI> <Output mask>
  
  
4. Bias field correction
^^^^^^^^^^^^^^^^^^^^^^^^
DWI bias field correction is perfomed by first estimating a correction field from the DWI b=0 image, then applying the field to correct all DW volumes. This can be done in a single step using the :code:`dwibiascorrect` script in MRtrix. The script uses bias field correction algorthims available in ANTS or FSL. In our experience the `N4 algorithm <http://www.ncbi.nlm.nih.gov/pmc/articles/PMC3071855/>`_ in ANTS gives superiour results. To install N4 install the `ANTS <http://stnava.github.io/ANTs/>`_ to package, then run perform bias field correction on DW images using::

    dwibiascorrect -ants -mask <Input brain mask> <Input DWI> <Output correction DWI>
    
5. Computing a group average response function
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^



  
  
  