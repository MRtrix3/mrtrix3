.. _response_function_estimation:

Response function estimation
============================

A prerequisite for spherical deconvolution is obtaining response
function(s), which is/are used as the kernel(s) by the deconvolution
algorithm. For the white matter, the response function is the signal
expected for a voxel containing a single, coherently oriented bundle
of axons. In case of multi-tissue variants of spherical deconvolution,
response functions for other tissue types may be introduced as well;
the most common ones being grey matter and/or CSF.

In MRtrix3, the :ref:`dwi2response` script offers a range of algorithms
to estimate these response function(s) directly from your dataset itself.
This process of estimating response function(s) from the data is
non-trivial. No single algorithm works for *any* possible scenario,
although some have proven to be more widely applicable than others.

Quick advice
------------

These algorithms appear to perform well in a wide range of scenarios,
based on experience and testing from both developers and the
`MRtrix3 community <http://community.mrtrix.org>`__.:

-  If you intend to perform *single-tissue* spherical deconvolution,
   the ``tournier`` algorithm is a convenient and reliable way to
   estimate the single-fibre white matter response function:

   ::

      dwi2response tournier dwi.mif wm_response.txt

-  If you intend to perform *multi-tissue* spherical deconvolution,
   the ``dhollander`` algorithm is a convenient and reliable way to
   estimate the single-fibre white matter response function as well
   as the grey matter and CSF response functions:

   ::

      dwi2response dhollander dwi.mif wm_response.txt gm_response.txt csf_response.txt

In general, it's always worthwhile checking your response function(s):

::

   shview wm_response.txt
      
Use the left and right arrow (keyboard) keys in this viewer to switch
between the different b-values ('shells') of the response function, if
it has more than one (this would for example be the case for the outputs
of the ``dhollander`` algorithm).

It may also be helpful to check which voxels were selected by the
algorithm to estimate the response function(s) from. For any
:ref:`dwi2response` algorithm, this can be done by adding the ``-voxels``
option, which outputs an image of these voxels. For example, for
the ``tournier`` algorithm:

::

   dwi2response tournier dwi.mif wm_response.txt -voxels voxels.mif
      
The resulting ``voxels.mif`` image can be overlaid on the ``dwi.mif``
dataset using the :ref:`mrview` image viewer for further inspection.


More information
----------------

Looking at the process of response function estimation in full detail,
there are four crucial steps. For each of these, I will also briefly
mention the typical process used.

1. Select those image voxels that are to be used when determining the
   response function - the 'single-fibre mask'. *Typical*: Varies.

2. Estimate the direction of the underlying fibres in each voxel.
   *Typical*: Often the diffusion tensor fit is still used for this
   purpose; though CSD itself can also be used as long as an initial
   response function estimate is available.

3. Rotate the signal measured in each single-fibre voxel in such a way
   that the estimated fibre direction coincides with the z-axis.
   *Typical*: This may be done by rotating the diffusion gradient table
   according to the estimated fibre direction; or if the diffusion
   signal is converted to spherical harmonics, then a spherical
   convolution can be used.

4. Combine these signals to produce a single response function.
   *Typical*: The ``m=0`` terms of the spherical harmonic series (which
   are rotationally symmetric about the z-axis) are simply averaged
   across single-fibre voxels.

Of these steps, the first is the one that has caused the greatest
difficulty, and is also the principle mechanism where the provided
response function estimation algorithms vary. It will therefore be the
primary focus of this document, though note that the other aspects of
this process may also change with ongoing development.

``dwi2response`` algorithms
---------------------------

``dhollander``
^^^^^^^^^^^^^^

TODO: Thijs is working on this documentation page.

``fa``
^^^^^^

In the previous version of MRtrix ('0.2'), the following heuristic was
suggested in the documentation for deriving the response function:

-  Erode a brain mask by a few voxels, to omit any voxels near the edge
   of the brain;

-  Select those voxels within the mask that have a Fractional Anisotropy
   (FA) of 0.7 or greater;

-  The ``estimate_response`` command would then be used to generate a
   response function, which would internally perform diffusion tensor
   estimation to get the fibre directions as well as the gradient
   reorientation.

Rather than this series of commands, ``dwi2response`` now provides a
similar heuristic in-built as the ``fa`` algorithm. The primary
difference is that by default, it will instead select the 300 voxels
with the highest FA (though this can be modified at the command-line).

This algorithm is provided partly for nostalgic purposes, but it also
highlights the range of possibilities for single-fbre voxel selection.
One of the problems associated with this approach (over and above the
feeling of uncleanliness from using the tensor model) is that in white
matter regions close to CSF, Gibbs ringing can make the signal in *b=0*
images erroneously low, which causes an artificial increase in FA, and
therefore such voxels get included in the single-fibre mask.

``manual``
^^^^^^^^^^

This algorithm is provided for cases where none of the available
algorithms give adequate results, for deriving multi-shell multi-tissue
response functions in cases where the voxel mask for each tissue must be
defined manually, or for anyone who may find it useful if trying to
devise their own mechanism for response function estimation. It requires
manual definition of both the single-fibre voxel mask (or just a voxel
mask for isotropic tissues); the fibre directions can also be provided
manually if necessary (otherwise a tensor fit will be used).

``msmt_5tt``
^^^^^^^^^^^^

This algorithm is intended for deriving multi-shell, multi-tissue
response functions that are compatible with the new Multi-Shell
Multi-Tissue (MSMT) CSD algorithm. The response function estimation
algorithm is identical to that described in `the
manuscript <http://linkinghub.elsevier.com/retrieve/pii/S1053-8119(14)00644-2>`__:
As long as EPI inhomogeneity field correction has been performed, and a
tissue-segmented anatomical image (prepared in the 5TT format for
:ref:`ACT <act>`) is provided with good
prior rigid-body alignment to the diffusion images, then these
high-resolution tissue segmentations can be used to identify
single-tissue voxels in the diffusion images. This algorithm is
hard-wired to provide response functions for the most typical use case
for MSMT CSD: An isotropic grey matter response, an anisotropic white
matter response, and an isotropic CSF response; the output response
functions are provided in the format expected by the :ref:`dwi2fod`
command. Those wishing to experiment with different multi-tissue
response function configurations will need to use the ``manual``
algorithm (which will provide a multi-shell response function if the
input DWI contains such data).

For reference, this algorithm operates as follows:

1. Resample the 5TT segmented image to diffusion image space.

2. For each of the three tissues (WM, GM, CSF), select those voxels that
   obey the following criteria:

-  The tissue partial volume fraction must be at least 0.95.

-  For GM and CSF, the FA must be no larger than 0.2.

3. For WM, use the mask derived from step 2 as the initialisation to the
   ``tournier`` algorithm, to select single-fibre voxels.

4. Derive a multi-shell response for each tissue for each of these three
   tissues. For GM and CSF, use *lmax=0* for all shells.

``tax``
^^^^^^^

This algorithm is a fairly accurate reimplementation of the approach
proposed by `Tax et
al. <http://www.sciencedirect.com/science/article/pii/S1053811913008367>`__.
The operation of the algorithm can be summarized as follows:

1. Initialise the response function using a relatively 'fat' profile,
   and the single-fibre mask using all brain voxels.

2. Perform CSD in all single-fibre voxels.

3. Exclude from the single-fibre voxel mask those voxels where the
   resulting FOD detects more than one discrete fibre population, e.g.
   using the ratio of the amplitudes of the first and second tallest
   peaks.

4. Re-calculate the response function using the updated single-fibre
   voxel mask.

5. Return to step 2, repeating until some termination criterion is
   achieved.

The following are the differences between the implementation in
``dwi2response`` and this manuscript:

-  Deriving the initial response function. In the manuscript, this is
   done using a tensor model with a low FA. I wasn't fussed on this
   approach myself, in part because it's difficult to get the correct
   intensity sscaling. Instead, the script examines the mean and
   standard deviation of the raw DWI volumes, and derives an initial
   *lmax=4* response function based on these.

-  The mechanism used to identify the peaks of the FOD. In
   ``dwi2response``, the FOD segmentation algorithm described in the
   `SIFT paper (Appendix
   2) <http://www.sciencedirect.com/science/article/pii/S1053811912011615>`__
   is used to locate the FOD peaks. The alternative is to use the
   :ref:`sh2peaks` command, which uses a Newton search from 60 pre-defined
   directions to locate these peaks. In my experience, the latter is
   slower, and may fail to identify some FOD peaks because the seeding
   directions are not sufficiently dense.

For the sake of completeness, the following are further modifications
that were made to the algorithm as part of the earlier ``dwi2response``
*binary*, but have been removed from the script as it is now provided:

-  Rather than using the ratio of amplitudes between the tallest and
   second-tallest peaks, this command instead looked at the ratio of the
   AFD of the largest FOD lobe, and the sum of the AFD of all other
   (positive) lobes in the voxel. Although this in some way makes more
   sense from a physical perspective (comparing the volume occupied by
   the primary fibre bundle to the volume of 'everything else'), it's
   possible that due to the noisy nature of the FODs at small
   amplitudes, this may have only introduced variance into the
   single-fibre voxel identification process. Therefore the script has
   reverted to the original & simpler peak amplitude ratio calculation.

-  A second, more stringent pass of SF voxel exclusion was performed,
   which introduced two more criteria that single-fibre voxels had to
   satisfy:

-  Dispersion: A measure of dispersion of an FOD lobe can be derived as
   the ratio between the integral (fibre volume) and the peak amplitude.
   As fibre dispersion increases, the FOD peak amplitude decreases, but
   the fibre volume is unaffected; therefore this ratio increases. The
   goal here was to explicitly exclude voxels from the single-fibre mask
   if significant orientation dispersion was observed; this can be taken
   into account somewhat by using the FOD peak amplitudes (as
   orientation dispersion will decrease the amplitude of the tallest
   peak), but from my initial experimentation I wanted something more
   stringent. However as before, given the difficulties that many users
   experienced with the ``dwi2response`` command, this algorithm in the
   new script errs on the side of simplicity, so this test is not
   performed.

-  Integral: By testing only the ratio of the tallest to second-tallest
   FOD peak amplitude, the absolute value of the peak amplitude is
   effectively ignored. This may or may not be considered problematic,
   for either small or large FOD amplitudes. If the peak amplitude / AFD
   is smaller than that of other voxels, it's possible that this voxel
   experiences partial volume with CSF: this may satisfy the peak ratio
   requirement, but using such a voxel is not ideal in response function
   estimation as its noise level will be higher and the Rician noise
   bias will be different. Conversely, both in certain regions of the
   brain and in some pathologies, some voxels can appear where the AFD
   is much higher due to T2 shine-through; it may seem appealing to use
   such voxels in response function estimation as the SNR is higher, but
   as for the low-signal case, the Rician noise bias will be different
   to that in the rest of the brain. The previous ``dwi2response``
   binary attempted to exclude such voxels by looking at the mean and
   standard deviation of AFD within the single-fibre mask, and excluding
   voxels above or below a certain threshold. As before, while this
   heuristic may or may not seem appropriate depending on your point of
   view, it has been excluded from the new ``dwi2response`` script to
   keep things as simple as possible.

``tournier``
^^^^^^^^^^^^

Independently and in parallel, Donald also developed a newer method for
response function estimation based on CSD itself; it was used in `this
manuscript <http://dx.doi.org/10.1002/nbm.3017>`__. It bears some
resemblance to the ``tax`` algorithm, but relies on a threshold on the
number of voxels in the single-fibre mask, rather than the ratio between
tallest and second-tallest peaks. The operation is as follows:

1. Define an initial response function that is as sharp as possible
   (ideally a flat disk, but will be fatter due to spherical harmonic
   truncation). Limit this initial function to *lmax=4*, as this makes
   the FODs less noisy in the first iteration.

2. Run CSD for all voxels within the mask (initially, this is the whole
   brain).

3. Select the 300 'best' single-fibre voxels. This is not precisely the
   ratio between tallest and second-tallest peaks; instead, the
   following equation is used, which also biases toward selection of
   voxels where the tallest FOD peak is larger:
   ``sqrt(|peak1|) * (1 - |peak2| / |peak1|)^2``. Use these voxels to
   generate a new response fuction.

4. Test to see if the selection of single-fibre voxels has changed; if
   not, the script is completed.

5. Derive a mask of voxels to test in the next iteration. This is the
   top 3,000 voxels according to the equation above, and dilated by one
   voxel.

6. Go back to step 2.

This approach appears to be giving reasonable results for the datasets
on which it has been tested. However if you are involved in the
processing of non-human brain images in particular, you may need to
experiment with the number of single-fibre voxels as the white matter is
typically smaller.

Writing your own algorithms
---------------------------

TODO: Thijs is working on this documentation page.

