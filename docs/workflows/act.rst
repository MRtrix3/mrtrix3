.. _act:

Anatomically-Constrained Tractography (ACT)
====

This page describes the recommended processing steps for taking advantage of the Anatomically-Constrained Tractography (ACT) framework, the image format used, and the commands available for manipulating these data.   There are also instructions for anyone looking to make use of alternative tissue segmentation approaches.

References
----

For full details on ACT, please refer to the following journal article:

    `Smith, R. E., Tournier, J.-D., Calamante, F., & Connelly, A. (2012). Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. NeuroImage, 62(3), 1924–1938. doi:10.1016/j.neuroimage.2012.06.005 <http://www.ncbi.nlm.nih.gov/pubmed/22705374/>`_

If you use ACT in your research, please cite the article above in your manuscripts.


Pre-processing steps
----

DWI distortion correction
^^^^

For the anatomical information to be incorporated accurately during the tractography reconstruction process, any geometric distortions present in the diffusion images must be corrected. The FSL 5.0 commands ``topup`` and ``eddy`` are effective in performing this correction based on a reversed phase-encode acquisition, though their interfaces can be difficult to figure out. Our current strategy is to acquire a pair of *b=0* images, the first with phase encode A>>P and the second P>>A, followed by the DWI acquisition with phase encode A>>P; the provided script ``revpe_distcorr`` interfaces with FSL to perform the distortion correction in this particular case. For alternative acquisitions, appropriate modification of the ``revpe_distcorr`` script may be an effective way to handle this processing step.

Image registration
^^^^

My personal preference is to register the T1-contrast anatomical image to the diffusion image series before any further processing of the T1 image is performed. By registering the T1 image to the diffusion series rather than the other way around, reorientation of the diffusion gradient table is not necessary; and by doing this registration before subsequent T1 processing, any subsequent images derived from the T1 are inherently aligned with the diffusion image series. This registration should be rigid-body only; if the DWI distortion correction is effective, a higher-order registration is likely to only introduce errors.

DWI pre-processing
^^^^

Because the anatomical image is used to limit the spatial extent of streamlines propagation rather than a binary mask derived from the diffusion image series, I highly recommend dilating the DWI brain mask prior to computing FODs; this is to make sure that any errors in derivation of the DWI mask do not leave gaps in the FOD data within the brain white matter, and therefore result in erroneous streamlines termination.

Tissue segmentation
^^^^

So far I have had success with using FSL tools to also perform the anatomical image segmentation; FAST is not perfect, but in most cases it's good enough, and most alternative software I tried provided binary mask images only, which is not ideal. The ``act_anat_prepare_fsl`` script interfaces with FSL to generate the necessary image data from the raw T1 image, using BET, FAST and FIRST. Note that this script automatically crops the inferior quarter of the image, as this was necessary with our T1 data for BET to operate correctly; if your anatomical images do not extent this far inferior, you may need to modify the script. See the '5TT format' section below for more details if you wish to use other segmentation software.

Using ACT
----

Once the necessary pre-processing steps are completed, using ACT is simple: just provide the tissue-segmented image to the ``tckgen`` command using the ``-act`` option.

In addition, since the propagation and termination of streamlines is primarily handled by the 5TT image, it is no longer necessary to provide a mask using the ``-mask`` option. In fact, for whole-brain tractography, it is recommend that you _not_ provide such an image when using ACT: depending on the accuracy of the DWI brain mask, its inclusion may only cause erroneous termination of streamlines inside the white matter due to exiting this mask. If the mask encompasses all of the white matter, then its inclusion does not provide any additional information to the tracking algorithm.

The 5TT format
----

When the ACT framework is invoked, it expects the tissue information to be provided in a particular format; this is referred to as the 'five-tissue-type (5TT)' format. This is a 4D, 32-bit floating-point image, where the dimension of the fourth axis is 5; that is, there are five 3D volumes in the image. These five volumes correspond to the different tissue types. In all brain voxels, the sum of these five volumes should be 1.0, and outside the brain it should be zero. The tissue type volumes must appear in the following order for the anatomical priors to be applied correctly during tractography:

0. Cortical grey matter
1. Sub-cortical grey matter
2. White matter
3. CSF
4. Pathological tissue

The first four of these are described in the ACT NeuroImage paper. The fifth can be optionally used to manually delineate regions of the brain where the architecture of the tissue present is unclear, and therefore the type of anatomical priors to be applied are also unknown. For any streamline entering such a region, *no anatomical priors are applied* until the streamline either exists that region, or stops due to some other streamlines termination criterion.

The following binaries are provided for working with the 5TT format:

* ``5tt2gmwmi``: Produces a mask image suitable for seeding streamlines from the grey matter - white matter interface (GMWMI). The resulting image should then be provided to the ``tckgen`` command using the ``seed_gmwmi`` option.
* ``5tt2vis``: Produces a 3D greyscale image suitable for visualisation purposes.
* ``5ttedit``: Allows the user to edit the tissue segmentations. Useful for manually correcting tissue segmentations that are known to be erroneous (e.g. dark blobs in the white matter being labelled as grey matter); see the command's help page for more details.
* ``5ttgen``: A simple worker command that is invoked by the ``act_anat_prepare_fsl`` script. Only likely to be of any use if the user wishes to utilise tissue segmentation results from alternative software.

Alternative tissue segmentation software
----

Users who wish to experiment with using tissue segmentations from different software sources are encouraged to do so; if a particular approach is shown to be effective we can add an appropriate script to MRtrix. A second script ``act_anat_prepare_freesurfer`` is also provided to demonstrate how the output of different software can be manipulated to provide the tissue segmentations in the appropriate format. It is however not recommended to actually use this alternative script for patient studies; many midbrain structures are not segmented by FreeSurfer, so the tracking may not behave as desired.

When producing a 5TT image from some other software source, the following requirements should be adhered to:

* The image should use a 32-bit floating-point data type, with image intensities (partial volume fractions) ranging from 0.0 to 1.0.
* The image should be four-dimensional, and the dimension of the fourth axis should be 5.
* The partial volume images for each tissue type should appear in the order as described earlier. If a particular tissue type is not provided at all (e.g. pathological tissue), the image volume must still be present, but should be zero-filled.
* For all brain voxels, the sum of partial volume fractions across all tissue types should be 1.0; outside the brain, it should be 0.0. The ``5ttgen`` command may be useful in ensuring that this criterion is upheld.
