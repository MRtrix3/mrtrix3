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





General recommendations
-----------------------

Choice of algorithm
^^^^^^^^^^^^^^^^^^^

While many algorithms exist, the following appear to perform well in a wide
range of scenarios, based on experience and testing from both developers and
the `MRtrix3 community <http://community.mrtrix.org>`__:

- **CSD:** If you intend to perform *single-tissue* :ref:`csd` (i.e. via
  ``dwi2fod csd``), the tournier_ algorithm is a convenient and reliable way to
  estimate the single-fibre white matter response function:
  
  .. code-block:: console

     dwi2response tournier dwi.mif wm_response.txt

  Other options include the fa_ or tax_ algorithms.

- **MSMT-CSD or global tractography:** If you intend to perform a
  *multi-tissue* analysis, such as :ref:`msmt_csd` (i.e. via ``dwi2fod
  msmt_csd``) or :ref:`global_tractography` (i.e. via ``tckglobal``), the
  dhollander_ algorithm is a convenient and reliable way to estimate the
  single-fibre white matter response function as well as the grey matter and
  CSF response functions:
  
  .. code-block:: console

     dwi2response dhollander dwi.mif wm_response.txt gm_response.txt csf_response.txt

  Another option is the msmt_5tt_ algorithm.

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

This algorithm currently is the original implementation of the strategy proposed in
[Dhollander2016]_ to estimate multi b-value (single-shell + b=0, or
multi-shell) response functions of single-fibre white matter (*anisotropic*),
grey matter and CSF (both *isotropic*), which can subsequently be used for
multi-tissue (constrained) spherical deconvolution algorithms.  It has the
distinct advantage of requiring *only* the DWI data as input, in contrast to
other multi-tissue response function estimations methods, making it the
simplest and most accessible method, and a sensible default for applications
that require multi-shell responses. 

This algorithm relies on an unsupervised algorithm, leveraging the relative
diffusion properties of the 3 tissue response functions with respect to each
other, to select the most appropriate voxels from which to estimate the
response functions.  It has been used succesfully in a wide range of conditions
(overall data quality, pathology, developmental state of the subjects,
animal data and ex-vivo data). Additional insights into a few specific
aspects of its performance can be found in [Dhollander2018a]_ In almost all
cases, it runs and performs well out of the box.  In exceptional cases where
the anisotropy in the data is particularly low (*very* early development,
ex-vivo data with low b-value, ...), it may be advisable to set the ``-fa``
parameter lower than its default value (of 0.2).  See [Dhollander2018b]_ for an
example of a dataset where changing this parameter was required to obtain the
best results.

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
close to CSF, for example, Gibbs ringing can severely affect FA values.
More advanced iterative strategies, such as the tournier_ and tax_
algorithms have been proposed more recently.

For more information, refer to the
:ref:`fa algorithm documentation <dwi2response_fa>`.




manual
^^^^^^

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
segment are then extracted using the tournier_ algorithm to do so (in contrast
to original publication, see `Replicating original publications`_ below)

The input tissue segmentation can be estimated using the same :ref:`pre-processing
pipeline <act_preproc>` as required for :ref:`act`, namely: correction for
motion and (EPI and other) distortions present in the diffusion MR data,
registration of the structural to (corrected) EPI data, and spatial
segmentation of the anatomical image.  This process is therefore dependent on
the accuracy of each of these steps, so that the T1 image can be reliably used
to select pure-tissue voxels in the DWI volumes.  Failure to achieve these may
result in inappropriate voxels being used for response function estimation,
with concomitant errors in tissue estimates.

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
to overcome some of the issues believed to be the cause of these
instabilities (see some discussion on this topic
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
tax_ algorithm, it was adjusted to overcome some occasional instabilities
and suboptimal solutions resulting from the latter. Notable differences
between the tournier_ and tax_ algorithms include:

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
below are not necessarily *exactly* as published, but close approximations
nonetheless. Any differences in implementation are highlighted where relevant.


Spherical deconvolution and Constrained spherical deconvolution
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In the original spherical deconvolution [Tournier2004]_ and constrained
spherical deconvolution [Tournier2007]_ papers, the response function was
estimated by extracting the 300 voxels with highest anisotroy within a brain
mask, eroded to avoid noisy voxels near the edge of the brain. This can be
performed using the fa_ method directly:

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
For the WM response, a further FA threshold was used.  This pipeline can be
most closely replicated using the :ref:`5ttgen` command and msmt_5tt_ algorithm
in this fashion:

.. code-block:: console

  5ttgen fsl T1.mif 5tt.mif
  dwi2response msmt_5tt dwi.mif 5tt.mif wm_response.txt gm_response.txt csf_response.txt -wm_algo fa

where:

- ``T1.mif`` is a coregistered T1 data set from the same subject (input)

- ``5tt.mif`` is the resulting tissue type segmentation, used subsequently used in the response function estimation (output/input)

- ``dwi.mif`` is the same dwi data set as used above (input)

- ``<tissue>_response.txt`` is the tissue-specific response function as used above (output)

One difference between these instructions and the methods as described in
the respective publications is that instead of selecting WM single-fibre voxels
_using an absolute FA threshold of 0.7 or 0.75, the 300 voxels with the highest FA
value inside the WM segmentation are used.



-------

 
.. [Dhollander2016] T. Dhollander, D. Raffelt, and A. Connelly. 
   *Unsupervised 3-tissue response function estimation from single-shell or multi-shell diffusion MR data without a co-registered T1 image.* 
   ISMRM Workshop on Breaking the Barriers of Diffusion MRI (2016), pp. 5 
   [`full text link <https://www.researchgate.net/publication/307863133_Unsupervised_3-tissue_response_function_estimation_from_single-shell_or_multi-shell_diffusion_MR_data_without_a_co-registered_T1_image>`__\ ]

.. [Dhollander2018a] T. Dhollander, D. Raffelt, and A. Connelly.
   *Accuracy of response function estimation algorithms for 3-tissue spherical deconvolution of diverse quality diffusion MRI data.*
   Proceedings of the International Society of Magnetic Resonance in Medicine, 26th annual meeting (2018) Paris, France.
   [`full text link <https://www.researchgate.net/publication/324770874_Accuracy_of_response_function_estimation_algorithms_for_3-tissue_spherical_deconvolution_of_diverse_quality_diffusion_MRI_data>`__\ ]

.. [Dhollander2018b] T. Dhollander, J. Zanin, B.A. Nayagam, G. Rance, and A. Connelly.
   *Feasibility and benefits of 3-tissue constrained spherical deconvolution for studying the brains of babies.*
   Proceedings of the International Society of Magnetic Resonance in Medicine, 26th annual meeting (2018) Paris, France.
   [`full text link <https://www.researchgate.net/publication/324770875_Feasibility_and_benefits_of_3-tissue_constrained_spherical_deconvolution_for_studying_the_brains_of_babies>`__\ ]

.. [Jeurissen2014] 1. B. Jeurissen, J.D. Tournier, T. Dhollander, A. Connelly, and J.  Sijbers. 
   *Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data.* 
   NeuroImage, 103 (2014), pp. 411–426 [`SD link <http://www.sciencedirect.com/science/article/pii/S1053811914006442>`__\ ]

.. [Tax2014] C.M.W. Tax, B. Jeurissen, S.B.Vos, M.A. Viergever, and A. Leemans.
   *Recursive calibration of the fiber response function for spherical deconvolution of diffusion MRI data.*
   NeuroImage, 86 (2014), pp. 67-80 [`SD link <https://www.sciencedirect.com/science/article/pii/S1053811913008367>`__\ ]

.. [Tournier2004] J-D. Tournier, F. Calamante, D.G. Gadian, and A. Connelly.
   *Direct estimation of the fiber orientation density function from diffusion-weighted MRI data using spherical deconvolution.*
   NeuroImage, 23 (2004), pp. 1176-85 [`SD link <https://www.sciencedirect.com/science/article/pii/S1053811904004100>`__\ ]

.. [Tournier2007] J-D. Tournier, F. Calamante, and A. Connelly.
   *Robust determination of the fibre orientation distribution in diffusion MRI: non-negativity constrained super-resolved spherical deconvolution.*
   Neuroimage, 35 (2007), pp. 1459-72. [`SD link <https://www.sciencedirect.com/science/article/pii/S1053811907001243?via%3Dihub>`__\ ]euroImage, 86 (2014), pp. 67-80 [`SD link <http://www.sciencedirect.com/science/article/pii/S1053811913008367>`__\ ]

.. [Tournier2013] J.D. Tournier, F. Calamante, and A. Connelly.
   *Determination of the appropriate b value and number of gradient directions for high-angular-resolution diffusion-weighted imaging.*
   NMR Biomed., 26 (2013), pp.  1775-86 [`Wiley link <https://onlinelibrary.wiley.com/doi/abs/10.1002/nbm.3017>`__\ ]
