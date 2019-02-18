.. _response_function_estimation:

Response function estimation
============================

A prerequisite for spherical deconvolution is obtaining the response
function(s), which is/are used as the kernel(s) by the deconvolution
algorithm. For the white matter, the response function models the signal
expected for a voxel containing a single, coherently oriented bundle
of axons [Tournier2004]_ [Tournier2007]_. In case of multi-tissue
variants of spherical deconvolution, response functions for other
tissue types are introduced as well; typically to represent grey
matter(-like) and/or CSF(-like) signals [Jeurissen2014]_ [Dhollander2016a]_.

In MRtrix3, the :ref:`dwi2response` script offers a range of algorithms
to estimate these response function(s) directly from your dataset itself.
This process of estimating response function(s) from the data is
non-trivial. No single algorithm works for *any* possible scenario,
although some have proven to be more widely applicable than others.


General recommendations
-----------------------

Choice of algorithm
^^^^^^^^^^^^^^^^^^^

While many algorithms exist, the following appear to perform well in a wide
range of scenarios, based on experience and testing from both developers and
the `MRtrix3 community <http://community.mrtrix.org>`__:

**Single-tissue CSD:** If you intend to perform (single-tissue)
:ref:`constrained_spherical_deconvolution` (e.g. via ``dwi2fod csd``),
the tournier_ algorithm is a convenient and reliable way to estimate
the single-fibre white matter response function:

.. code-block:: console

   dwi2response tournier dwi.mif wm_response.txt

Other options include the fa_ or tax_ algorithms.

**Multi-tissue CSD or global tractography:** If you intend to perform a
*multi-tissue* analysis, such as :ref:`msmt_csd` (e.g. via ``dwi2fod
msmt_csd``) or :ref:`global_tractography` (e.g. via ``tckglobal``), the
dhollander_ algorithm is a convenient and reliable way to estimate the
single-fibre white matter response function as well as the grey matter and
CSF response functions:

.. code-block:: console

   dwi2response dhollander dwi.mif wm_response.txt gm_response.txt csf_response.txt

Other options include the msmt_5tt_ algorithm.

Checking the results
^^^^^^^^^^^^^^^^^^^^

In general, it's always worthwhile checking your response function(s):

.. code-block:: console

   shview wm_response.txt

Use the left and right arrow (keyboard) keys in this viewer to switch
between the different b-values ('shells') of the response function, if
it has more than one b-value (this would for example be the case for
the outputs of the dhollander_ algorithm).

It may also be helpful to check which voxels were selected by the
algorithm to estimate the response function(s) from. For any
:ref:`dwi2response` algorithm, this can be done by adding the ``-voxels``
option, which outputs an image of these voxels. For example, for
the tournier_ algorithm:

.. code-block:: console

   dwi2response tournier dwi.mif wm_response.txt -voxels voxels.mif

The resulting ``voxels.mif`` image can be overlaid on the ``dwi.mif``
dataset using the :ref:`mrview` image viewer for further inspection.



Available algorithms
--------------------

The available algorithms differ in a few general properties, related
to what they deliver (as output) and require (as input), notably

-  **single- versus multi-tissue**: whether they only estimate a
   single-fibre white matter response function (tournier_, tax_
   and fa_) or also additional response functions for other tissue
   types (dhollander_ and msmt_5tt_ both output a single-fibre
   white matter response function as well as grey matter and CSF
   response functions)

-  **single versus multiple b-values**: whether they only output
   response function(s) for a single b-value (tournier_, tax_
   and fa_) or for all—or a selection of— b-values (dhollander_
   and msmt_5tt_)

-  **input requirements**: whether they only require the DWI dataset
   as input (tournier_, dhollander_, tax_ and fa_) or
   also additional input(s) (msmt_5tt_ requires a 5TT segmentation
   from a spatially aligned anatomical image)

Beyond these general categories, the algorithms differ mostly in the actual
strategy used to determine the voxels that will be used to estimate
the response function(s) from.

The manual_ choice is an exception to most of the above, in that it
allows/*requires* you to provide the voxels yourself, and even allows
you to provide single-fibre orientations manually as well. It should
only be considered in case of exceptional kinds of data, or otherwise
exceptional requirements. Caution is advised with respect to *interpretation*
of spherical deconvolution results using manually defined response
function(s).

The following sections provide more details on each algorithm specifically.



dhollander
^^^^^^^^^^

This algorithm is the official implementation of the strategy proposed
in [Dhollander2016b]_ (including improvements proposed in [Dhollander2019]_)
to estimate multi b-value (single-shell + b=0, or multi-shell) response functions
for single-fibre white matter (*anisotropic*), grey matter and CSF (both
*isotropic*), which can subsequently be used for multi-tissue (constrained)
spherical deconvolution algorithms.  It has the distinct advantage of requiring
*only* the DWI data as input, in contrast to other multi-tissue response function
estimation methods, making it the simplest and most accessible method, and a
sensible default for applications that require multi-tissue responses.

This is a fully automated unsupervised algorithm that leverages the relative
diffusion properties of the 3 tissue response functions with respect to each
other, across all b-values and the angular domain, to select the most appropriate
voxels from which to estimate the response functions.  It has been used
successfully in a wide range of conditions (overall data quality, pathology,
developmental state of the subjects, animal data and ex-vivo data). Additional
insights into its performance are presented in [Dhollander2018a]_. Due to its
ability to deal with the presence of extensive white matter (hyperintense)
lesions, it was for example also successfully used in [Mito2018a]_. The response
functions as obtained in this particular way also form the basis of the 3-tissue
framework to study the microstructure of lesions and other
pathology [Dhollander2017]_ [Mito2018b]_.

The algorithm has been further improved in [Dhollander2019]_. While the 2016 version
identified the voxels to estimate the single-fibre white matter response function
using the tournier_ algorithm, the new 2019 version relies on a novel strategy
that optimises these voxels using properties of the signal across all b-values (and
the full angular domain). It's also faster than the original approach.

In almost all cases, the algorithm runs and performs well out of the box.
In *exceptional* cases where the anisotropy in the data is particularly *low*
(*very* early development, ex-vivo data, (with) low b-value, ...), it is *sometimes*
advisable to set the ``-fa`` parameter *lower* than its default value of 0.2.
See [Dhollander2018b]_ for a good example of a dataset where changing this
parameter was required to obtain good results.  This FA threshold should be set
so as to roughly separate the bulk of WM from the rest (GM and CSF).  Further
imperfections are corrected by the algorithm itself during a later stage.

As always, check the ``-voxels`` option output in unusually (challenging) cases.

For more information, refer to the
:ref:`dhollander algorithm documentation <dwi2response_dhollander>`.



fa
^^

This algorithm is an implementation of the strategy proposed in [Tournier2013]_
to estimate a single b-value (single-shell) response function of single-fibre
white matter, which can subsequently be used for single-tissue (constrained)
spherical deconvolution. The algorithm estimates this response function from
the 300 voxels with the highest FA value in an eroded brain mask. There are
also options to change this number or provide an absolute FA threshold.

Due to relying *only* on FA values, this strategy is relatively
limited in its abilities to select the best voxels. In white matter
close to CSF, for example, Gibbs ringing can affect FA values.
More advanced iterative strategies, such as the tournier_ and tax_
algorithms have been proposed more recently.

For more information, refer to the
:ref:`fa algorithm documentation <dwi2response_fa>`.




manual
^^^^^^

This algorithm is provided for cases where none of the available automated
algorithms give adequate results, for deriving multi-shell multi-tissue
response functions in cases where the voxel mask for each tissue must be
defined manually, or for anyone who may find it useful if trying to
devise their own mechanism for response function estimation. It requires
manual definition of both the single-fibre voxel mask (or just a voxel
mask for isotropic tissues); the fibre directions can also be provided
manually if necessary (otherwise a tensor fit will be used).

For more information, refer to the
:ref:`manual algorithm documentation <dwi2response_manual>`.




msmt_5tt
^^^^^^^^

This algorithm is a reimplementation of the strategy proposed in
[Jeurissen2014]_ to estimate multi b-value response functions of single-fibre
white matter (*anisotropic*), grey matter and CSF (both *isotropic*), which can
subsequently be used for multi-tissue (constrained) spherical deconvolution.
The algorithm is primarily driven by a prior (:ref:`5TT`) tissue segmentation,
typically obtained from a spatially aligned anatomical image. This also
requires prior correction for susceptibility-induced (EPI) distortions of the
DWI dataset. The algorithm selects voxels with a segmentation partial volume of
at least 0.95 for each tissue type.  Grey matter and CSF are further
constrained by an (upper) 0.2 FA threshold. Single-fibre voxels within the WM
segment are then extracted using the tournier_ algorithm (in contrast
to original publication, see `Replicating original publications`_ below).

The input tissue segmentation can be estimated using the same :ref:`pre-processing
pipeline <act_preproc>` as required for :ref:`act`, namely: correction for
motion and (EPI and other) distortions present in the diffusion MR data,
registration of the structural to (corrected) EPI data, and spatial
segmentation of the anatomical image.  This process is therefore also dependent on
the accuracy of each of these steps, so that the T1 image can be reliably used
to select pure-tissue voxels in the DWI volumes.  Failure to achieve high
accuracy for each of these individual steps may result in inappropriate voxels
being used for response function estimation, with concomitant errors in tissue estimates.

The dhollander_ algorithm does not rely on a number of these steps. A comparison
is presented in [Dhollander2018a]_.

For further information, refer to the
:ref:`msmt_5tt algorithm documentation <dwi2response_msmt_5tt>`.




tax
^^^

This algorithm is a reimplementation of the iterative approach proposed in
[Tax2014]_ to estimate a single b-value (single-shell)
response function of single-fibre white matter, which can subsequently be used
for single-tissue (constrained) spherical deconvolution. The algorithm iterates
between performing CSD and estimating a response function from all voxels
detected as being 'single-fibre' from the CSD result itself. The criterion for
a voxel to be 'single-fibre' is based on the ratio of the amplitude of second
tallest to the tallest peak. The method is initialised with a 'fat' response
function; i.e., a response function that is safely deemed to be much less
'sharp' than the true response function.

This algorithm has occasionally been found to be unstable and converge
towards suboptimal solutions. The tournier_ algorithm has been engineered
with the intention to overcome some of the issues believed to be the
cause of these instabilities (see some discussion on this topic
`here <https://github.com/MRtrix3/mrtrix3/issues/422>`__
and `here <https://github.com/MRtrix3/mrtrix3/pull/426>`__).

For more information, refer to the
:ref:`tax algorithm documentation <dwi2response_tax>`.





tournier
^^^^^^^^

This algorithm is a reimplementation of the iterative approach proposed in
[Tournier2013]_ to estimate a single b-value (single-shell)
response function of single-fibre white matter, which can subsequently be used
for single-tissue (constrained) spherical deconvolution. The algorithm iterates
between performing CSD and estimating a response function from a set of the
best 'single-fibre' voxels, as detected from the CSD result itself. Notable
differences between this implementation and the algorithm described in
[Tournier2013]_ include:

-  This implementation is initialised by a sharp lmax=4 response function
   as opposed to one estimated from the 300 brain voxels with the highest FA.

-  This implementation uses a more complex metric to measure how
   'single-fibre' FODs are: √|peak1| × (1 − \|peak2\| / \|peak1\|)²,
   as opposed to a simple ratio of the two tallest peaks. This new metric has
   a bias towards FODs with a larger tallest peak, to avoid favouring
   small, yet low SNR, FODs.

-  This implementation only performs CSD on the 3000 best 'single-fibre'
   voxels (of the previous iteration) at each iteration.

While the tournier_ algorithm has a similar iterative structure as the
tax_ algorithm, it was adjusted with the intention to overcome some
occasional instabilities and suboptimal solutions resulting from the
latter. Notable differences between the tournier_ and tax_ algorithms
include:

-  The tournier_ algorithm is initialised by a *sharp* (lmax=4) response
   function, while the tax_ algorithm is initialised by a *fat* response
   function.

-  This implementation of the tournier_ algorithm uses a more complex
   metric to measure how 'single-fibre' FODs are (see above), while the
   tax_ algorithm uses a simple ratio of the two tallest peaks.

-  The tournier_ algorithm estimates the response function at each
   iteration only from the 300 *best* 'single-fibre' voxels, while the
   tax_ algorithm uses *all* 'single-fibre' voxels.

Due to these differences, the tournier_ algorithm is currently believed to
be more robust in a wider range of scenarios (for further information on this
topic, refer to some of the discussions `here
<https://github.com/MRtrix3/mrtrix3/issues/422>`__
and `here <https://github.com/MRtrix3/mrtrix3/pull/426>`__).

For more information, refer to the
:ref:`tournier algorithm documentation <dwi2response_tournier>`.






Replicating original publications
---------------------------------

For completeness, we provide below instructions for replicating the approaches
used in previous relevant publications. Note that the implementations provided
below are not necessarily *exactly* as published, but aim to be close
approximations nonetheless.


Spherical deconvolution and Constrained spherical deconvolution
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In the original spherical deconvolution [Tournier2004]_ and constrained
spherical deconvolution [Tournier2007]_ papers, the response function was
estimated by extracting the 300 voxels with the highest FA values within a
brain mask, eroded to avoid noisy voxels near the edge of the brain. This
can be performed using the fa_ method directly:

.. code-block:: console

  dwi2response fa dwi.mif response.txt

where:

- ``dwi.mif`` is the input DWI data set,

- ``response.txt`` is the estimated response function, produced as output



MSMT-CSD and Global tractography 
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In the original multi-shell multi-tissue CSD [Jeurissen2014]_ and global
tractography [Christiaens2015]_ papers, response functions were estimated using
a prior tissue segmentation obtained from a coregistered structural T1 scan.
For the WM response, a further hard FA threshold was used: respectively 0.7 in
the MSMT-CSD paper and 0.75 in the global tractography paper. This pipeline can be
replicated using the :ref:`5ttgen` command and msmt_5tt_ algorithm with
the ``-sfwm_fa_threshold`` option in this fashion:

.. code-block:: console

  5ttgen fsl T1.mif 5tt.mif
  dwi2response msmt_5tt dwi.mif 5tt.mif wm_response.txt gm_response.txt csf_response.txt -sfwm_fa_threshold 0.7

where:

- ``T1.mif`` is a coregistered T1 data set from the same subject (input)

- ``5tt.mif`` is the resulting tissue type segmentation, used subsequently used in the response function estimation (output/input)

- ``dwi.mif`` is the same dwi data set as used above (input)

- ``<tissue>_response.txt`` is the tissue-specific response function as used above (output)

To replicate the global tractography paper, specify a value of 0.75
instead of 0.7 as shown in the command line above.


