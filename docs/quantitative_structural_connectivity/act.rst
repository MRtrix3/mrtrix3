.. _act:

Anatomically-Constrained Tractography (ACT)
===========================================

This page describes the recommended processing steps for taking advantage of the Anatomically-Constrained Tractography (ACT) framework, the image format used, and the commands available for manipulating these data.   There are also instructions for anyone looking to make use of alternative tissue segmentation approaches.

References
----------

For full details on ACT, please refer to the following journal article:

    `Smith, R. E., Tournier, J.-D., Calamante, F., & Connelly, A. (2012). Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. NeuroImage, 62(3), 1924–1938. doi:10.1016/j.neuroimage.2012.06.005 <http://www.ncbi.nlm.nih.gov/pubmed/22705374/>`_

If you use ACT in your research, please cite the article above in your manuscripts.


Pre-processing steps
--------------------

DWI distortion correction
^^^^^^^^^^^^^^^^^^^^^^^^^

For the anatomical information to be incorporated accurately during the tractography reconstruction process, any geometric distortions present in the diffusion images must be corrected. The FSL 5.0 commands ``topup`` and ``eddy`` are effective in performing this correction based on a reversed phase-encode acquisition, though their interfaces can be daunting. We therefore provide a wrapper script, ``dwipreproc``, which interfaces with these tools to perform correction of multiple forms of image distortion (motion, eddy current and inhomogeneity). Please read the :ref:`dwipreproc_page` page, and the :ref:`dwipreproc` help page for further details.

Image registration
^^^^^^^^^^^^^^^^^^

My personal preference is to register the T1-contrast anatomical image to the diffusion image series before any further processing of the T1 image is performed. By registering the T1 image to the diffusion series rather than the other way around, reorientation of the diffusion gradient table is not necessary; and by doing this registration before subsequent T1 processing, any subsequent images derived from the T1 are inherently aligned with the diffusion image series. This registration should be rigid-body only; if the DWI distortion correction is effective, a higher-order registration is likely to only introduce errors.

.. NOTE::

    Some software used for registration may, by default, automatically re-sample the moving image to the voxel grid of the static image. In the case of T1-to-_b_=0 image registration, this results in down-sampling of the T1 image. This is not only not ideal for ACT (which is designed to be able to exploit the higher spatial resolution of tissue segmentation), but may cause issues if trying to run tissue segmentation algorithms. For instance: ``5ttgen fsl`` (specifically the ``run_first_all`` component) regularly fails if provided with a T1 image that has been resampled to match the corresponding diffusion images. Therefore, regardless of whether image registration is performed before or after tissue segmentation in preparation for ACT, we recommend that you check that the spatial resolution of the T1 image has not decreased during tissue segmentation.

DWI pre-processing
^^^^^^^^^^^^^^^^^^

Because the anatomical image is used to limit the spatial extent of streamlines propagation rather than a binary mask derived from the diffusion image series, I highly recommend dilating the DWI brain mask prior to computing FODs; this is to make sure that any errors in derivation of the DWI mask do not leave gaps in the FOD data within the brain white matter, and therefore result in erroneous streamlines termination.

Tissue segmentation
^^^^^^^^^^^^^^^^^^^

So far I have had success with using FSL tools to also perform the anatomical image segmentation; FAST is not perfect, but in most cases it's good enough, and most alternative software I tried provided binary mask images only, which is not ideal. The ``5ttgen`` script using the ``fsl`` algorithm interfaces with FSL to generate the necessary image data from a T1 image, using BET, FAST and FIRST. Note that this script also crops the resulting image so that it contains no more than the extracted brain (as this reduces the file size and therefore improves memory access performance during tractography); if you want the output image to possess precisely the same dimensions as the input T1 image, you can use the ``-nocrop`` option.

Using ACT
---------

Once the necessary pre-processing steps are completed, using ACT is simple: just provide the tissue-segmented image to the ``tckgen`` command using the ``-act`` option.

In addition, since the propagation and termination of streamlines is primarily handled by the 5TT image, it is no longer necessary to provide a mask using the ``-mask`` option. In fact, for whole-brain tractography, it is recommend that you _not_ provide such an image when using ACT: depending on the accuracy of the DWI brain mask, its inclusion may only cause erroneous termination of streamlines inside the white matter due to exiting this mask. If the mask encompasses all of the white matter, then its inclusion does not provide any additional information to the tracking algorithm.

The 5TT format
--------------

When the ACT framework is invoked, it expects the tissue information to be provided in a particular format; this is referred to as the 'five-tissue-type (5TT)' format. This is a 4D, 32-bit floating-point image, where the dimension of the fourth axis is 5; that is, there are five 3D volumes in the image. These five volumes correspond to the different tissue types. In all brain voxels, the sum of these five volumes should be 1.0, and outside the brain it should be zero. The tissue type volumes must appear in the following order for the anatomical priors to be applied correctly during tractography:

0. Cortical grey matter
1. Sub-cortical grey matter
2. White matter
3. CSF
4. Pathological tissue

The first four of these are described in the ACT NeuroImage paper. The fifth can be optionally used to manually delineate regions of the brain where the architecture of the tissue present is unclear, and therefore the type of anatomical priors to be applied are also unknown. For any streamline entering such a region, *no anatomical priors are applied* until the streamline either exists that region, or stops due to some other streamlines termination criterion.

The following binaries are provided for working with the 5TT format:

* ``5tt2gmwmi``: Produces a mask image suitable for seeding streamlines from the grey matter - white matter interface (GMWMI). The resulting image should then be provided to the ``tckgen`` command using the ``-seed_gmwmi`` option.
* ``5tt2vis``: Produces a 3D greyscale image suitable for visualisation purposes.
* ``5ttedit``: Allows the user to edit the tissue segmentations. Useful for manually correcting tissue segmentations that are known to be erroneous (e.g. dark blobs in the white matter being labelled as grey matter); see the command's help page for more details.

Alternative tissue segmentation software
----------------------------------------

Users who wish to experiment with using tissue segmentations from different software sources are encouraged to do so; if a particular approach is shown to be effective we can add an appropriate script to MRtrix. The ``5ttgen`` script has a second algorithm, ``freesurfer``, which demonstrates how the output of different software can be manipulated to provide the tissue segmentations in the appropriate format. It is however not recommended to actually use this alternative algorithm for patient studies; many midbrain structures are not segmented by FreeSurfer, so the tracking may not behave as desired.

Users who wish to try manipulating the tissue segmentations from some alternative software into the 5TT format may find it most convenient to make a copy of one of the existing algorithms within the ``scripts/src/_5ttgen//`` directory, and modify accordingly. The ``5ttgen`` script will automatically detect the presence of the new algorithm, and make it available at the command-line.

