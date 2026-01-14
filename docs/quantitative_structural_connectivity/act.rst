.. _act:

Anatomically-Constrained Tractography (ACT)
===========================================

This page describes the recommended processing steps for taking advantage of the Anatomically-Constrained Tractography (ACT) framework [Smith2012]_, the image format used, and the commands available for manipulating these data.   There are also instructions for anyone looking to make use of alternative tissue segmentation approaches.

.. _act_preproc:

Pre-processing steps
--------------------

DWI distortion correction
^^^^^^^^^^^^^^^^^^^^^^^^^

For the anatomical information to be incorporated accurately during the tractography reconstruction process, any geometric distortions present in the diffusion images must be corrected. The FSL 5.0 commands ``topup`` and ``eddy`` are effective in performing this correction based on a reversed phase-encode acquisition, though their interfaces can be daunting. We therefore provide a wrapper script, ``dwifslpreproc``, which interfaces with these tools to perform correction of multiple forms of image distortion (motion, eddy current and inhomogeneity). Please read the :ref:`dwifslpreproc_page` page, and the :ref:`dwifslpreproc` help page for further details.

Image registration
^^^^^^^^^^^^^^^^^^

The typical advice currently is,
having computed a registration between the diffusion image series and the T1-weighted image,
to apply the corresponding transformation to the DWI series during pre-processing.
By doing this registration as early as is reasonable,
any subsequent images derived from either the DWI series or the T1-weighted image are inherently aligned with one another.
This registration should be rigid-body only:
if the DWI distortion correction is effective,
a higher-order registration is likely to only introduce errors.

:: NOTE ..:

    Earlier versions of this documentation suggested transformation of the T1-weighted image to align with the DWI series.
    The justification for this process is that it obviates the danger of applying a spatial transformation to diffusion-weighted image data
    without also performing the requisite rotation of the diffusion gradient directions.
    At the time of update however,
    this recommendation has been *reversed*,
    for two reasons:

    -   If the ``mrtransform`` command is used to apply a linear transformation to DWI data,
        it will *automatically* apply the rotation component of that transformation
        to any diffusion gradient table embedded within the image header.

    -   Modern neuroimaging experiments are frequently multi-modal.
        In this context,
        it is often necessary to define a singular reference image to which all other image data are aligned.
        This is very likely to be a T1-weighted image.
        Transforming a T1-weighted image to align with the DWI voxel grid would therefore result
        in derivatives of both that transformed T1-weighted image and the DWI series
        not being spatially aligned with other modalities.

    There does however remain residual scope for this process to go awry:

    -   Do *not* apply a linear transform to a DWI series,
        and then interpret those data with respect to a diffusion gradient table
        that is stored in an *external text file in MRtrix format* rather than within the image header;
        failure to rotate that table according to the applied spatial transformation would result in erroneous outcomes.

    -   If relying on storage of the diffusion gradient table in the FSL ``bvecs`` / ``bvals`` format,
        those diffusion sensitisation directions rotate according to the rotation of the image voxel grid,
        such that they appropriately mimic the spatial transformation of the image data.
        This is however *only* appropriate if the *strides of the output image match the input image*.
        If this is violated for any reason
        ---for instance, by specifying not only a linear transformation to be applied but also a template voxel grid on which to resample the data---
        then the diffusion gradient table will no longer be suitable for analysis of those image data.


DWI pre-processing
^^^^^^^^^^^^^^^^^^

Historically, a brain mask derived from the DWI data themselves would be utilised to constrain the propagation of streamlines outside of the brain.
Such a mask could therefore also be used to limit the computational expense of processing steps such as FOD estimation.
ACT provides the prospect of altering this precedent,
given that delineation of the spatial extent of the brain could be more reliably determined from the T1-weighted image data
owing to its higher spatial resolution and tissue contrast.
It is suggested that *if* one chooses to make use of a mask to prevent the estimation of FODs where they are not needed,
then that mask should be *more generous* than one's estimate of the spatial extent of the brain,
for two reasons:

1.  If a DWI-derived brain mask erroneously under-estimates the extent of the brain,
    then this could cause premature termination of streamlines within the brain
    when the extent of the brain is defined based on the tissue segmentation derived from the anatomical image.
    This would be more deleterious to data quality
    than would be estimation of FODs outside of that region.

2.  When streamline propagation occurs,
    fibre orientations are derived for sub-voxel locations through *interpolation*.
    This means that a streamline vertex may reside within a voxel that is included in a mask,
    but the interpolation that defines the next direction of traversal at that sub-voxel locations
    may be determined making use of image data from voxels *not* included in that mask.
    For this reason it may be beneficial
    ---even technically if not utilising ACT---
    for fibre orientations to be estimated based on a mask that has gone through at least two dilations
    (necessary to ensure that for any sub-voxel location inside of the intended mask,
    valid image data are present for all 8 voxels in the trilinear interpolation neighbourhood).

Tissue segmentation
^^^^^^^^^^^^^^^^^^^

The ``5ttgen`` command provides a common interface to many algorithms
for taking tissue segmentations from a high-resolution anatomical image
(sometimes pre-computed, sometimes computed internally from the image data),
and producing a tissue segmentation image in the format as expected for ACT
(see 5TT_ below).

-   The ``5ttgen fsl`` algorithm is what was presented in the ACT article.
-   The ``5ttgen freesurfer`` algorithm is exceptionally fast
    (discounting the computational expense required to initially run FreeSurfer);
    but because the tissue segmentation is a *hard* segmentation
    ---that is, each voxel consists of exactly one tissue type only---
    the resulting image does not provide continuous *partial volume fractions*,
    such that there is minimal useful sub-voxel interpolation information.
    This can lead to ugly ''stair-stepping'' artifacts in the resulting tractograms,
    where streamline terminations follow the pattern of the voxel grid
    rather than the smooth tissue interfaces.
-   The ``5ttgen hsvs`` algorithm is the most advanced and generally recommended algorithm.
    It is however only applicable to *in vivo* human data
    given that it is predicated on FreeSurfer (and also typically FSL FIRST).
    It is also the most likely to undergo ongoing development.
-   If particular tissue segmentation algorithms are advantageous in particular contexts,
    and the use of such derivative data for ACT could be similarly beneficial,
    researchers are encouraged to develop their own ``5ttgen`` algorithms,
    in which case they can be made available to the wider research community.
    This simply involves duplicating one of the existing files in directory ``lib/mrtrix3/_5ttgen/``
    and modifying the underlying content as appropriate;
    the ``5ttgen`` script will *automatically* detect the presence of the new algorithm
    and make it available at the command-line.

Note that by default,
``5ttgen`` will *crop* the tissue segmentation image
so that its spatial extent is no larger than is necessary to encapsulate the non-zero data.
This is done to reduce file size
and additionally to benefit computational performance during streamline propagation.
If there is instead justification on the part of the researcher
that the resulting image should have *exactly* the same voxel grid as the input image,
then this final cropping step can be bypassed using the ``-nocrop`` option.

A computed tissue segmentation image
will typically have the same spatial resolution
as whatever high-resolution anatomical-contrast image it was computed from,
and is typically derived independently of any DWI data.
This means that the tissue segumentation image and the DWI data
(or derivatives thereof)
will reside on *different voxel grids*
(in terms of spatial resolution and/or orientations of image axes).
As communicated in the original ACT article,
this is a *positive attribute* of the implementation:
each source of image data preserves its native spatial resolution
and does not undergo any unecessary resampling
during which interpolation invariably introduces some degree of image blurring.
As streamlines are propagated,
sub-voxel interpolation is performed *independently* for ACT image data and DWI data,
such that their residing on a common voxel grid is not necessary.
It is therefore generally *discouraged* to explicitly apply an intervention
to place these data onto a common voxel grid,
given that doing so is unnecessary and may have deleterious consequences.

Using ACT
---------

Once the necessary pre-processing steps are completed, using ACT is simple: just provide the tissue-segmented image to the ``tckgen`` command using the ``-act`` option.

In addition, since the propagation and termination of streamlines is primarily handled by the 5TT image, it is no longer necessary to provide a mask using the ``-mask`` option. In fact, for whole-brain tractography, it is recommend that you _not_ provide such an image when using ACT: depending on the accuracy of the DWI brain mask, its inclusion may only cause erroneous termination of streamlines inside the white matter due to exiting this mask. If the mask encompasses all of the white matter, then its inclusion does not provide any additional information to the tracking algorithm.

.. _5TT:

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
* ``5ttcheck``: Check that one or more input images conform to the 5TT format.
* ``5ttedit``: Allows the user to edit the tissue segmentations. Useful for manually correcting tissue segmentations that are known to be erroneous (e.g. dark blobs in the white matter being labelled as grey matter); see the command's help page for more details.
