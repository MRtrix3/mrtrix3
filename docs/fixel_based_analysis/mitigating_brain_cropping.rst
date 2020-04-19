.. _mitigating_brain_cropping:

Mitigating the effects of brain cropping
========================================

In some datasets, the DWI images do not provide coverage of the entire brain
cerebrum and cerebellum, due to poor placement of the imaging field of view
and/or subject movement leading to that area of the brain shifting outside the FoV
for at least one DWI volume. In such cases, it would be erroneous to infer a
change in any fixel-wise metric in that area if subjects for which no valid image
data are available were to contribute to the assessment. It is therefore necessary
to properly track where each subject possesses valid image data and where they do
not, and to modify the processing of and inference from such data accordingly.

The way to achieve this at the point of statistical inference is for the fixel data
to contain the value NaN (Not A Number) in any location where a valid quantitative
metric could not be obtained for that particular subject. Such data are removed
from the GLM on a fixel-by-fixel basis. This technique is explained in greater
detail and demonstrated in [Smith2019a]_.

The following instructions describe a way to modify to a typical Fixel-Based Analysis
pipeline, in order to ensure that any location in the template image where image data
for a particular subject may have been affected by such cropping will contain the
value NaN. It is however recommended that you manually inspect the results of these
processing steps in order to ensure that the manipulations of the data are operating
as intended.

1. Replace zero-filled values in the DWI with NaN

   When the FSL command eddy (invoked by *MRtrix3*'s :ref:`dwifslpreproc`) cannot
   reconstruct valid image data for all DWI volumes in a particular voxel, it fills
   that voxel with zero values. The following sequence of commands identifies such
   voxels, and replaces the values stored within those voxels with NaN::

      mrmath dwi.mif norm - -axis 3 | mrthreshold - - -abs 0.0 -comparison gt -nan | mrcalc dwi.mif - -mult dwi_nan.mif

   Note that this command must be run *after* `dwifslpreproc`, but *before* any
   upsampling of the DWI data: the latter introduces an interpolation step, such that
   some voxels in the upsampled image will be decreased in intensity due to this effect,
   but will not be precisely zero. 

   If data upsampling is performed subsequent to this step, regions of the image
   containing these NaN values will become *larger*. This occurs because any
   voxels for which performing 3D interpolation will attempt to sample from an
   input voxel containing the value NaN will itself obtain a value of NaN.

2. Ignore other instructions elsewhere regarding brain masks

   Filling voxels outside of the brain with values of NaN achieves a comparable effect
   to providing a brain mask: voxels containing such values will not contribute to
   various calculations just as though they were to lie outside of a provided brain mask.
   As such, explicitly providing a brain mask that does not exclude any voxels not
   already excluded by step 1 would not have any consequence. 

3. Use NaN fill value in :ref:`mrtransform`

   When transforming subject FOD data to template space, instruct the :ref:`mrtransform`
   command to fill voxels in template space outside of the input image FoV with the
   value NaN, rather than zeroes (using the :code:`-nan` option); this will ensure that
   any voxel in template space for which valid subject data are not available will
   contain the value NaN, regardless of which step in the pipeline led to that fact.

4. Substitute template mask with number of valid subjects

   Upon generation of the study-specific population template, the intersection
   of all subject brain masks in template space will *not* be utilised. Indeed it is
   not entirely appropriate to transform individual subjects' brain masks to template
   space, as the results of such would not reflect the propagation of NaN values
   described at the end of point 1.

   It may however instead be useful to know, for each voxel in template space, how
   many subjects possess valid image data in that location::

      for_each * : mrconvert IN/fod_in_template_space_NOT_REORIENTED.mif -coord 3 0 -axes 0,1,2 - "|" mrcalc - -finite IN/valid_data_template_space_mask.mif -datatype bit
      mrmath */valid_data_template_space_mask.mif sum ../template/valid_data_num_subjects.mif

   It may then be useful to apply a threshold to this image (:ref:`mrthreshold`)
   in order to inform the derivation of a voxel mask for statistical inference;
   e.g. one may wish to exclude altogether from analysis those voxels with less
   than some number of subjects; this choice is left open to the researcher.

5. Propagate NaN values to fixel quantitative metrics

   For voxels in template space for which no valid data are available for a particular
   subject, we want fixel data to contain the value NaN rather than 0.0. This is done
   by projecting the voxel mask representing those voxels for which valid subject data
   are available into the template fixel mask, and then modifying the fixel values
   accordingly::

      mkdir ../template/valid_data_masks/
      for_each * : voxel2fixel IN/valid_data_template_space_mask.mif ../template/fd/ ../template/valid_data_masks/ PRE.mif

      mkdir ../template/fd_nan/
      cp ../template/fd/index.mif ../template/fd/directions.mif ../template/fd_nan/
      for_each * : mrcalc ../template/fd/PRE.mif ../template/valid_data_masks/PRE.mif -div ../template/fd_nan/PRE.mif

   This is performed *after* the :ref:`fixelcorrespondence` step, and must be
   performed independently for each fixel metric of interest.

6. When runing :code:`fixelcfestats`, the presence of NaN values in the input data
   will be detected automatically, and this fact will be reported to the user.

   This command should be expected to take approximately 4 to 5 times longer to
   complete than typical usage where all input data are finite.
