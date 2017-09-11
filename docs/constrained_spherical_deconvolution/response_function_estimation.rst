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
   
Beyond these general categories, the algorithms differ mostly in how
they derive the voxels that will be used to estimate the response
function(s) from, and to a lesser extent also how they derive the fibre
orientation for single-fibre voxels.

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

TODO: Thijs is working on this documentation section.

For more information, refer to the
:ref:`dhollander algorithm documentation <dwi2response_dhollander>`.

'fa' algorithm
^^^^^^^^^^^^^^

This algorithm is an implementation of the strategy proposed in
`Tournier et al. (2004) <http://www.sciencedirect.com/science/article/pii/S1053811904004100>`__
to estimate a single b-value (single-shell) response function for
single-fibre white matter, which can subsequently be used in single-tissue
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

For more information, refer to the
:ref:`msmt_5tt algorithm documentation <dwi2response_msmt_5tt>`.

'tax' algorithm
^^^^^^^^^^^^^^^

This algorithm is a reimplementation of the iterative approach proposed in
`Tax et al. (2014) <http://www.sciencedirect.com/science/article/pii/S1053811913008367>`__
to estimate a single b-value (single-shell) response function for
single-fibre white matter, which can subsequently be used in single-tissue
(constrained) spherical deconvolution. The algorithm iterates between
performing CSD and estimating a response function from all voxels detected
as being `single-fibre` from the CSD result itself. The criterium for
a voxel to be `single-fibre` is based on the ratio of the amplitude of
second tallest to the first tallest peak. The method is initialised with
a 'fat' response function; i.e., a response function that is safely deemed
to be much less 'sharp' than the true response function.

This algorithm has occasionally been found to behave unstable and converge
towards suboptimal solutions. The ``tournier`` algorithm has been engineered
to overcome some of the issues believed to be the cause of these
instabilities (see some discussion on this topic
`here <https://github.com/MRtrix3/mrtrix3/issues/422>`__
and `here <https://github.com/MRtrix3/mrtrix3/pull/426>`__).

For more information, refer to the
:ref:`tax algorithm documentation <dwi2response_tax>`.

'tournier' algorithm
^^^^^^^^^^^^^^^^^^^^

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

For more information, refer to the
:ref:`tournier algorithm documentation <dwi2response_tournier>`.

Writing your own algorithms
---------------------------

TODO: Thijs is working on this documentation section. Will suggest ``manual``
as a first (easier) option, and (python) implementation of a ``dwi2response``
algorithm as another (and mention in which folder the algos sit).

