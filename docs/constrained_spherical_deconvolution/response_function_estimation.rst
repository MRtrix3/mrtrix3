.. _response_function_estimation:

Response function estimation
============================

A prerequisite for spherical deconvolution is obtaining (the) response
function(s), which is/are used as the kernel(s) by the deconvolution
algorithm. For the white matter, the response function models the signal
expected for a voxel containing a single, coherently oriented bundle
of axons. In case of multi-tissue variants of spherical deconvolution,
response functions of other tissue types are introduced as well;
typically to represent grey matter(-like) and/or CSF signals.

In MRtrix3, the :ref:`dwi2response` script offers a range of algorithms
to estimate these response function(s) directly from your dataset itself.
This process of estimating response function(s) from the data is
non-trivial. No single algorithm works for *any* possible scenario,
although some have proven to be more widely applicable than others.

Quick advice
------------

The following algorithms appear to perform well in a wide range of
scenarios, based on experience and testing from both developers and
the `MRtrix3 community <http://community.mrtrix.org>`__:

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
it has more than one b-value (this would for example be the case for
the outputs of the ``dhollander`` algorithm).

It may also be helpful to check which voxels were selected by the
algorithm to estimate the response function(s) from. For any
:ref:`dwi2response` algorithm, this can be done by adding the ``-voxels``
option, which outputs an image of these voxels. For example, for
the ``tournier`` algorithm:

::

   dwi2response tournier dwi.mif wm_response.txt -voxels voxels.mif

The resulting ``voxels.mif`` image can be overlaid on the ``dwi.mif``
dataset using the :ref:`mrview` image viewer for further inspection.

All available algorithms
------------------------

The available algorithms differ in a few general properties, related
to what they deliver (as output) and require (as input), notably

-  **single- versus multi-tissue**: whether they only estimate a
   single-fibre white matter response function (``tournier``, ``tax``
   and ``fa``) or also additional response functions for other tissue
   types (``dhollander`` and ``msmt_5tt`` both output a single-fibre
   white matter response function as well as grey matter and CSF
   response functions)

-  **single versus multiple b-values**: whether they only output
   response function(s) for a single b-value (``tournier``, ``tax``
   and ``fa``) or for all—or a selection of— b-values (``dhollander``
   and ``msmt_5tt``)

-  **input requirements**: whether they only require the DWI dataset
   as input (``tournier``, ``dhollander``, ``tax`` and ``fa``) or
   also additional input(s) (``msmt_5tt`` requires a 5TT segmentation
   from a spatially aligned anatomical image)

Beyond these general categories, the algorithms differ mostly in the actual
strategy used to determine the voxels that will be used to estimate
the response function(s) from.

The ``manual`` choice is an exception to most of the above, in that it
allows/*requires* you to provide the voxels yourself, and even allows
you to provide single-fibre orientations manually as well. It should
only be considered in case of exceptional kinds of data, or otherwise
exceptional requirements. Caution is advised with respect to *interpretation*
of spherical deconvolution results using manually defined response
function(s).

The following sections provide more details on each algorithm specifically.

'dhollander' algorithm
^^^^^^^^^^^^^^^^^^^^^^

This algorithm is the original implementation of the strategy proposed in
`Dhollander et al. (2016) <https://www.researchgate.net/publication/307863133_Unsupervised_3-tissue_response_function_estimation_from_single-shell_or_multi-shell_diffusion_MR_data_without_a_co-registered_T1_image>`__
to estimate multi b-value (single-shell + b=0, or multi-shell) response
functions of single-fibre white matter (*anisotropic*), grey matter
and CSF (both *isotropic*), which can subsequently be used for
multi-tissue (constrained) spherical deconvolution algorithms.
This is an unsupervised algorithm that only requires the diffusion
weighted dataset as input. It leverages the relative diffusion properties
of the 3 tissue response functions with respect to each other, allowing it
to select the best voxels to estimate the response functions from.

The algorithm has been succesfully tested in a wide range of conditions
(overall data quality, pathology, developmental state of the subjects,
animal data and ex-vivo data).  In almost all cases, it runs and performs
well out of the box.  In exceptional cases where the anisotropy in the
data is particularly low (*very* early development, ex-vivo data with low
b-value, ...), it may be advisable to set the ``-fa`` parameter lower
than its default value (of 0.2).  As always, check the ``-voxels`` option
output in unusually (challenging) cases.


For more information, refer to the
:ref:`dhollander algorithm documentation <dwi2response_dhollander>`.

'fa' algorithm
^^^^^^^^^^^^^^

This algorithm is an implementation of the strategy proposed in
`Tournier et al. (2004) <http://www.sciencedirect.com/science/article/pii/S1053811904004100>`__
to estimate a single b-value (single-shell) response function of
single-fibre white matter, which can subsequently be used for single-tissue
(constrained) spherical deconvolution. The algorithm estimates this
response function from the 300 voxels with the highest FA value in an
eroded brain mask. There are also options to change this number or
provide an absolute FA threshold.

Due to relying *only* on FA values, this strategy is relatively
limited in its abilities to select the best voxels. In white matter
close to CSF, for example, Gibbs ringing can severely affect FA values.
More advanced iterative strategies, such as the ``tournier`` and ``tax``
algorithms have been proposed in the mean time.

For more information, refer to the
:ref:`fa algorithm documentation <dwi2response_fa>`.

'manual' algorithm
^^^^^^^^^^^^^^^^^^

This algorithm is provided for cases where none of the available
algorithms give adequate results, for deriving multi-shell multi-tissue
response functions in cases where the voxel mask for each tissue must be
defined manually, or for anyone who may find it useful if trying to
devise their own mechanism for response function estimation. It requires
manual definition of both the single-fibre voxel mask (or just a voxel
mask for isotropic tissues); the fibre directions can also be provided
manually if necessary (otherwise a tensor fit will be used).

For more information, refer to the
:ref:`manual algorithm documentation <dwi2response_manual>`.

'msmt_5tt' algorithm
^^^^^^^^^^^^^^^^^^^^

This algorithm is a reimplementation of the strategy proposed in
`Jeurissen et al. (2014) <http://www.sciencedirect.com/science/article/pii/S1053811914006442>`__
to estimate multi b-value response functions of single-fibre
white matter (*anisotropic*), grey matter and CSF( both *isotropic*),
which can subsequently be used for multi-tissue (constrained) spherical
deconvolution. The algorithm is primarily driven by a prior ('5TT')
tissue segmentation, typically obtained from a spatially aligned anatomical
image. This also requires prior correction for susceptibility-induced (EPI)
distortions of the DWI dataset. The algorithm selects voxels with a
segmentation partial volume of at least 0.95 for each tissue type.
Grey matter and CSF are further constrained by an (upper) 0.2 FA threshold.
A notable difference between this implementation and the algorithm described in
`Jeurissen et al. (2014) <http://www.sciencedirect.com/science/article/pii/S1053811914006442>`__
is the criterion to extract single-fibre voxels from the white matter
segmentation: this implementation calls upon the ``tournier`` algorithm
to do so, while the paper uses a simple (lower) 0.7 FA threshold.

Due to the challenge of accurately aligning an anatomical image (e.g.
T1-weighted image) with the diffusion data, including correction for distortions
up to an accuracy on the order of magnitude of the spatial resolution of
the diffusion image, as well as accurate spatial segmentation, this
algorithm has more prerequisites than the ``dhollander`` algorithm.

For more information, refer to the
:ref:`msmt_5tt algorithm documentation <dwi2response_msmt_5tt>`.

'tax' algorithm
^^^^^^^^^^^^^^^

This algorithm is a reimplementation of the iterative approach proposed in
`Tax et al. (2014) <http://www.sciencedirect.com/science/article/pii/S1053811913008367>`__
to estimate a single b-value (single-shell) response function of
single-fibre white matter, which can subsequently be used for single-tissue
(constrained) spherical deconvolution. The algorithm iterates between
performing CSD and estimating a response function from all voxels detected
as being 'single-fibre' from the CSD result itself. The criterion for
a voxel to be 'single-fibre' is based on the ratio of the amplitude of
second tallest to the tallest peak. The method is initialised with a
'fat' response function; i.e., a response function that is safely deemed
to be much less 'sharp' than the true response function.

This algorithm has occasionally been found to be unstable and converge
towards suboptimal solutions. The ``tournier`` algorithm has been engineered
to overcome some of the issues believed to be the cause of these
instabilities (see some discussion on this topic
`here <https://github.com/MRtrix3/mrtrix3/issues/422>`__
and `here <https://github.com/MRtrix3/mrtrix3/pull/426>`__).

For more information, refer to the
:ref:`tax algorithm documentation <dwi2response_tax>`.

'tournier' algorithm
^^^^^^^^^^^^^^^^^^^^

This algorithm is a reimplementation of the iterative approach proposed in
`Tournier et al. (2013) <http://onlinelibrary.wiley.com/doi/10.1002/nbm.3017/abstract>`__
to estimate a single b-value (single-shell) response function of
single-fibre white matter, which can subsequently be used for single-tissue
(constrained) spherical deconvolution. The algorithm iterates between
performing CSD and estimating a response function from a set of the best
'single-fibre' voxels, as detected from the CSD result itself. Notable differences
between this implementation and the algorithm described in `Tournier et al. (2013)
<http://onlinelibrary.wiley.com/doi/10.1002/nbm.3017/abstract>`__ include:

-  This implementation is initialised by a sharp lmax=4 response function
   as opposed to one estimated from the 300 brain voxels with the highest FA.

-  This implementation uses a more complex metric to measure how
   'single-fibre' FODs are: √|peak1| × (1 − \|peak2\| / \|peak1\|)²,
   as opposed to a simple ratio of the two tallest peaks. This new metric has
   a bias towards FODs with a larger tallest peak, to avoid favouring
   small, yet low SNR, FODs.

-  This implementation only performs CSD on the 3000 best 'single-fibre'
   voxels (of the previous iteration) at each iteration.

While the ``tournier`` algorithm has a similar iterative structure as the
``tax`` algorithm, it was adjusted to overcome some occasional instabilities
and suboptimal solutions resulting from the latter. Notable differences
between the ``tournier`` and ``tax`` algorithms include:

-  The ``tournier`` algorithm is initialised by a *sharp* (lmax=4) response
   function, while the ``tax`` algorithm is initialised by a *fat* response
   function.

-  This implementation of the ``tournier`` algorithm uses a more complex
   metric to measure how 'single-fibre' FODs are (see above), while the
   ``tax`` algorithm uses a simple ratio of the two tallest peaks.

-  The ``tournier`` algorithm estimates the response function at each
   iteration only from the 300 *best* 'single-fibre' voxels, while the
   ``tax`` algorithm uses *all* 'single-fibre' voxels.

Due to these differences, the ``tournier`` algorithm is currently believed to
be more robust and accurate in a wider range of scenarios (for further
information on this topic, refer to some of the discussions
`here <https://github.com/MRtrix3/mrtrix3/issues/422>`__
and `here <https://github.com/MRtrix3/mrtrix3/pull/426>`__).

For more information, refer to the
:ref:`tournier algorithm documentation <dwi2response_tournier>`.

