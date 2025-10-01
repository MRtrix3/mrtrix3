.. _dwi_masking:

DWI masking
===========

Many DWI processing operations either necessitate the use of a binary mask
image in order to spatially constrain the operation in some way, or can be
executed in less time by not performing the relevant calculations in those
voxels that are not of interest. However, while this would seem like a
relatively trivial operation, it is in fact deceptively difficult to
devise an appropriate heuristic for deriving an appropriate mask that works
for a wide range of DWI data. It is not uncommon for the derivation of this
mask to go awry in a range of scenarios, which can have serious implications
for downstream processing steps. For this reason, as of MRtrix version
``3.1.0``, a *range* of DWI mask derivation algorithms are provided, allowing
users to assess which heuristics work best for their particular data. The
purpose of this documentation page is to describe those algorithms that are
available, the circumstances in which they may or may not work, the features
that are available for users to manipulate this behaviour, and the
applications in which these details are most relevant for user attention.

``dwi2mask`` algorithms
-----------------------

``dwi2mask 3dautomask``
^^^^^^^^^^^^^^^^^^^^^^^

Provides the mean *b=0* image directly to AFNI_ `command 3dAutomask <https://afni.nimh.nih.gov/pub/dist/doc/program_help/3dAutomask.html>`_ .

``dwi2mask ants``
^^^^^^^^^^^^^^^^^

Provides the mean *b=0* image directly to ANTs_ command
``antsBrainExtraction.sh``, configured for T2-weighted image input.

This algorithm necessitates the specification of a template image and
corresponding binary mask image defined on that template. These two images
must be provided by the user either using the ``-template`` command-line
option, or the ``Dwi2maskTemplateImage`` and ``Dwi2maskTemplateMask``
configuration file options (see :ref:`dwi2mask_config`).

``dwi2mask b02template``
^^^^^^^^^^^^^^^^^^^^^^^^

Registers the subject's mean *b=0* image to a template image, and
back-propagates a binary mask back into the individual's DWI voxel grid.
Achieved as follows:

1. Non-linearly register subject's mean *b=0* image to a specified template
   image;

2. *(If not calculated implicitly as part of step 1)* Invert the non-linear
   deformation field;

3. Transform binary mask associated with template image onto voxel grid of
   mean *b=0* image (with interpolation);

4. Apply a threshold of 0.5 to transformed image to produce a mask.

There are multiple external software tools that can be utilised for performing
the core image registration and transformation processes:

-  ``antsquick``: Utilises the ANTs_ command ``antsRegistrationSyNQuick.sh``
   for registration; transforms mask image to subject space using ANTs_
   command ``antsApplyTransforms``.

-  ``antsfull``: Utilises the ANTs_ commands ``antsRegistration``
   for registration, using the registration parameters specified in the article:

   Tustison, Nicholas J., and Brian B. Avants.
   Explicit B-Spline Regularization in Diffeomorphic Image Registration.
   Frontiers in Neuroinformatics 7 (December 23, 2013): 39.
   https://doi.org/10.3389/fninf.2013.00039.

   Template mask image is then transformed to subject space using ANTs_
   command ``antsApplyTransforms``.

-  ``fsl``: Utilises FSL_ commands as follows:

   -  flirt_: Initial affine registration;
   -  fnirt_: Non-linear registration;
   -  invwarp_: Inversion of warp from subject to template;
   -  applywarp_: Transform template mask to subject space.

By default, if no manual selection is made here using either the ``-software``
command-line option or the ``Dwi2maskTemplateSoftware`` configuration file
entry, the ``antsquick`` approach will be used.

This algorithm necessitates the specification of a template image and
corresponding binary mask image defined on that template. These two images
must be provided by the user either using the ``-template`` command-line
option, or the ``Dwi2maskTemplateImage`` and ``Dwi2maskTemplateMask``
configuration file options (see :ref:`dwi2mask_config`).

The registration operation can be expected to perform best if the specified
template image is of comparable shape and image contrast to that of the
*b=0* volumes of the DWI data being processed. As such, if using an existing
template image, a T2-weighted image would be recommended. Alternatively, one
could produce a population template *b=0* image based on one's own data, and
manually define a mask on that template that could then subsequently be
used for DWI masking.

For all registration algorithms, there are ``dwi2mask`` command-line options
available for fine-tuning the behaviour of the registration by passing
command-line options down to the relevant command(s); further, it is possible
to set such parameters within the MRtrix configuration file, which may be of
particular use if configuration file option ``Dwi2maskAlgorithm`` is set to
``b02template`` (see :ref:`dwi2mask_config`).

``dwi2mask consensus``
^^^^^^^^^^^^^^^^^^^^^^

This algorithm is unique compared to all other ``dwi2mask`` algorithms,
in that it does not provide one specific heuristic for DWI mask estimation;
instead, it executes *all other* ``dwi2mask`` algorithms, and produces a
single mask based on the consensus of those algorithms. Currently this
consensus is simply those voxels that were included in the estimated masks
of *more than 50%* of the algorithms utilised. Note that if the external
software requirements of any specific ``dwi2mask`` algorithm are not
installed, the ``consensus`` algorithm will report that not all algorithms
could be executed, and will utilise only the outputs of those algorithms
that *could* be executed successfully.

``dwi2mask fslbet``
^^^^^^^^^^^^^^^^^^^

Provides the mean *b=0* image directly to FSL_ command ``bet``.

``dwi2mask hdbet``
^^^^^^^^^^^^^^^^^^^

Provides the mean *b=0* image directly to HD-BET_ command ``hd-bet``.

``dwi2mask legacy``
^^^^^^^^^^^^^^^^^^^

Reproduces the behaviour of the ``dwi2mask`` binary executable that was
included in *MRtrix3* prior to version ``3.1.0``.

It involves the following steps:

1. Compute the mean diffusion-weighted signal intensity for each *b*-value;

2. For each *b*-value independently, automatically determine a threshold to
   apply to produce a binary mask;

3. Sum the masks from step 2 across *b*-values;

4. Apply a median filter;

5. Select the largest connected component and fill holes;

6. Apply mask cleaning filter to remove small areas only connected to the
   largest component via thin "bridges".

``dwi2mask mean``
^^^^^^^^^^^^^^^^^

A heuristic algorithm that is based on simply taking the mean DWI intensity
across all volumes, and then applying a threshold. It was reported to provide
good results for some forms of data, but is not necessarily guaranteed to do
so for other DWI acquisition protocols; algorithm ``dwi2mask trace`` is
intended to operate on a similar concept, but be more robust against variations in
acquisition.

Operations are as follows:

1. Compute the mean DWI intensity across all volumes, regardless of *b*-value;

2. Automatically determine an intensity threshold for this image to produce
   a binary mask;

3. Select the largest connected component and fill any holes;

4. Apply mask cleaning filter to remove small areas only connected to the
   largest component via thin "bridges".

``dwi2mask mtnorm``
^^^^^^^^^^^^^^^^^^^

This algorithm implements a subset of the functionalities provided in the
``dwibiasnormmask`` script (described in further detail below).
It is based on utilisation of the results generated by the ``mtnormalise`` command.
The basic premise is that, following multi-shell multi-tissue CSD and appropriate
response function bias correction / bias field cocrrection / intensity normalisation,
an image consisting of the sum of all macroscopic tissue ODFs should be approximately
1.0 in brain voxels and 0.0 in non-brain voxels.

The order of operations is as follows:

1. If not provided by the user, generate an initial brain mask using the default
   ``dwi2mask`` algorithm.

2. Perform three-tissue response function estimation.

3. Perform multi-shell multi-tissue CSD
   (with all three macroscopic tissues---WM, GM and CSF---if possible,
   otherwise only WM and CSF)

4. Use ``mtnormalise`` to correct:

   1. Biases in response function magnitudes using ``-balanced`` option
      (note that this functionality is *deliberately omitted* from typical
      quantitative analysis pipelines as it may regress out effects of interest)
   2. Smoothly-varying bias field
   3. Global intensity scaling

5. Calculate an image of the sum of tissue ODFs

6. Apply a threshold to binarize this image
   (default threshold is 0.5).

7. Apply mask cleaning operations (eg. largest connected component).

``dwi2mask synthstrip``
^^^^^^^^^^^^^^^^^^^^^^^

The SynthStrip_ method is based on a deep learning neural network that has been
trained on a wide range of neuroimaging modalities and data qualities. This
algorithm provides the mean *b*\=0 image to SynthStrip, whether installed as part
of FreeSurfer (version 7.3.0 or later) or as the stand-alone Singularity container.

``dwi2mask trace``
^^^^^^^^^^^^^^^^^^

Heuristic algorithms for generating masks from DWI data based on
trace-weighted images (i.e. mean image intensity within each shell)
in a manner different to that of the ``dwi2mask legacy`` algorithm.

Its behaviour is as follows:

1.  Calculate the trace-weighted image for each shell;

2.  For each shell, find a multiplicative factor that gives the trace-weighted
    image approximately the same intensity of that of the first shell
    (this is so that each shell contributes approximately equally
    toward determination of the mask);

3.  Calculate the mean trace-weighted image across shells;

4.  Automatically determine an intensity threshold for this image to produce
    a binary mask;

5.  Select the largest connected component and fill any holes;

6.  Apply mask cleaning filter to remove small areas only connected to the
    largest component via thin "bridges";

7.  If the command-line option ``-iterative`` is *not* used, the algorithm
    ceases at this point (i.e. the default behaviour);

8.  For each *b*-value shell, compute the mean and standard deviation of
    the trace-weighted image intensities inside and outside of the current
    mask, and use this to derive Cohen's *d* statistic;

9.  Perform a recombination of the trace-weighted images; but the
    multiplicative weights applied to each *b*-value shell trace image are,
    instead of being based on intensity matching as in step 2, the
    Cohen's *d* statistics calculated in step 8;

10. Apply a threshold and mask filtering operations as in steps 4--6;

11. If the resulting mask differs from the previous estimate, go back to
    step 8; if not, or if a maximum number of iterations is reached,
    the algorithm is completed.

Note that the iterative version of this algorithm can currently be considered
a hypothetical heuristic, and it is not yet known whether or not its behaviour
is reasonable across a range of DWI data; it should therefore be considered
entirely experimental.

.. _dwi2mask_algorithm_comparison:

Algorithm comparison
--------------------

.. csv-table::
   :header: "Algorithm", "External dependencies", "Uses more that *b=0*", "Assumptions", "Robust to bias field", "Can use GPU"
   :widths: auto

   "``3dAutomask``", "Yes (AFNI_)", "No", "Unknown", "Unknown", "No"
   "``ants``", "Yes (ANTs_)", "No", "Brain; WM darker than GM", "Unknown", "No"
   "``b02template``", "Yes (ANTs_ / FSL_)", "No", "Matches template", "Yes", "No"
   "``consensus``", "Only if installed", "Yes", "Various", "Various", "No"
   "``fslbet``", "Yes (FSL_)", "No", "Approx. spherical", "Yes", "No"
   "``hdbet``", "Yes (HD-BET_)", "No", "Human brain", "Yes", "Yes"
   "``legacy``", "No", "Yes", "Single connected component", "No", "No"
   "``mtnorm``", "No", "Yes", "WM / GM / CSF constituency; single connected component", "Yes", "No"
   "``synthstrip``", "Yes", "No", "Human brain", "Yes", "Yes"
   "``trace``", "No", "Yes", "Single connected component", "No", "No"

.. _dwi2mask_python:

Python scripts utilising ``dwi2mask``
-------------------------------------

There are a number of Python scripts provided within *MRtrix3* that
operate on DWI data and necessitate use of a mask, and therefore (if not
provided with one explicitly at the command-line) will internally execute
the ``dwi2mask`` command.

Because it is not possible for the user to manually specify how ``dwi2mask``
should be utilised in this scenario, there are
`configuration file options <../reference/config_file_options.html>`_
provided to assist in controlling the behaviour of ``dwi2mask`` in these
scenarios (see below).

.. csv-table::
    :header: "*MRtrix3* Python command", "Purpose of DWI mask"
    :widths: auto

    "``dwi2response``", "| Voxels outside of the initial mask are never considered as candidates for response function(s), nor do they contribute to any optimisation of the selection of such."
    "``dwibiascorrect``", "| Only voxels within the mask are utilised in optimisation of bias field parameters.
    | For ``ants`` algorithm, field is estimated within the mask but applied to all voxels within the field of view (field basis is extrapolated beyond the extremities of the mask);
    | for ``fsl`` algorithm, field is both estimated within, and applied to, only those voxels within the mask, producing a discontinuity in image intensity at the outer edge of the mask that can be deleterious for subsequent quantitative analyses."
    "``dwibiasnormmask``", "| Determination of an *initial* brain mask by which to constrain the first iteration (see below)."
    "``dwifslpreproc``", "| Constrains optimisation of distortion parameter estimates in FSL_ ``eddy``.
    | If performing susceptibility distortion correction, this is applied to the DWI data subsequently to the appplication of FSL_ command ``applytopup``."
    "``dwigradcheck``", "| Utilised as both seed and mask image for streamlines tractography in the ``tckgen`` command."

``dwibiasnormmask``
-------------------

This new script is an experimental approach for improving DWI brain mask estimation
(among other things), initially created during development of the MRtrix3_connectome_
BIDS App.
It is based on the simple observation that the processes of bias field estimation,
intensity normalisation, and brain mask derivation, can have circular dependencies
between one another, and that therefore combining them into a single step may be
beneficial.
It is however noted that the behaviour of this algorithm can vary between different
types of data, and therefore close scruitiny of such is recommended.

While this script is highly dependent on the operation of the ``mtnormalise`` command
(as was observed to be the case for the ``dwi2mask mtnorm`` algorithm above,
which performs a subset of the functionalities within ``dwibiasnormmask``),
the form of the primary results that it provides are slightly different:

-  *Output intended for usage*:

   With ``mtnormalise``, a set of ODFs are provided as input, and a set of ODFs
   are then yielded as output, where the output ODFs have been corrected for
   a smoothly-spatially-varying bias field, and global intensity scaling
   (and importantly for quantitative applications the same intensity scaling is
   applied to all ODFs).
   For ``dwibiasnormmask``, the provided input is a DWI series, and the yielded
   output is a DWI series, where the output has had the same smoothly-spatially-varying
   bias field and global intensity scaling corrections applied.
   The process of estimating these corrections is identical;
   the only difference is that in ``dwibiasnormmask`` the corrections are *back-projected*
   to correct the DWI series from which the ODFs were estimated,
   rather than directly utilising the corrected ODFs.

-  *Global intensity normalisation*:

   The topic of :ref:`global-intensity-normalisation` is a long-standing issue
   in the domain of CSD analysis.
   Unlike other diffusion models, the *b*\=0 intensity of each voxel is not
   used as a reference for the modelling of DWIs in that voxel,
   and even in the context of multi-tissue CSD the composition of the voxel is
   not explicitly forced to be unity.
   This does however raise the issue of how to appropriately globally scale the
   intensities of the image data in order for observed differences in eg.
   Apparent Fibre Density (AFD) to be attributable to the effect of interest
   rather then meaningless differential scaling of image intensities between subjects.

   The approach taken by ``mtnormalise`` is to determine the scaling factor that
   results in voxels throughout the brain having a sum of tissue densities of
   approximately unity. They will not all be exactly unity, even after bias field
   correction, but they should be approximately centred around unity.

   ``dwibiasnormmask`` provides a more advanced version of the original proposal
   for global intensity normalisation for AFD analysis.
   It was first proposed that the *b*\=0 signal intensity in CSF should act as a
   reference intensity to normalise across subjects.
   However identifying appropriate exemplar voxels to do so can be labour-intensive
   and difficult.
   In ``dwibiasnormmask``, information from the ``dwi2response dhollander`` response
   function estimation algorithm and the ``mtnormalise`` approach are combined in
   such a way that the *b*\=0 CSF-like intensity is scaled to a fixed reference intensity:

   -  using data from across the entire brain even in the presence of partial volume;
   -  accounting for potential miscalibrations in response function estimation;
   -  in conjunction with bias field estimation and correction.

   It is not yet known whether using this approach for global intensity normalisation
   may yield greater sensitivity to effects of interest.
   It should however be noted that if one were to subsequently execute ``mtnormalise``
   and make use of its output ODFs,
   then the effective global intensity normalisation behaviour would revert to
   that of ``mtnormalise`` rather than that described above.

Another key aspect of this algorithm is the data used to derive the brain mask.
Most DWI brain masking approaches base their operation on the mean *b*\=0 image
(see dwi2mask_algorithm_comparison_ above).
In this algorithm, there is an alternative 3D image that can be used to drive brain mask
derivation, being the sum of tissue ODFs.
Depending on the configuration, this image may be used rather than the bias-field-corrected
DWI series to estimate a new brain mask at each iteration;
for instance, duplicating the functionality of the ``dwi2mask mtnorm`` algorithm above.

The script itself operates as follows:

1. If no initial mask is provided, then one must be calculated using ``dwi2mask``.

2. Three-tissue response function estimation using ``dwi2response dhollander``.

3. Multi-shell multi-tissue CSD, by default using a lower WM *lmax* for computational efficiency.

4. ``mtnormalise`` to estimate bias field and intensity scaling factors between tissues.

5. Estimation of a new brain mask, using either the bias-field-corrected DWI series
   or the tissue ODF sum image.

6. Determination of whether to exit, or *loop back to step 2*, based on:

   1. Adequate similarity of the brain mask between successive iterations;
   2. Masks between successive iterations becoming less similar rather than more similar
      (indicating some form of instability or divergence);
   3. Reaching maximal number of iterations.

While the primary output of the command is the DWI series corrected for bias field and
intensity normalised, the brain mask corresponding to the last stable iteration can be
additionally exported using the ``-output_mask`` option.

When initially developed, the number of iterations for this approach was fixed at 2,
as the solution was found to erroneously diverge after that in some instances.
It is however possible that for certain data, as well as the subsequent addition of the
explicit check for mask divergence, it is possible to permit a larger number of iterations
and allow the algorithm to converge toward a fixed solution.
Feedback on the success or failure of this experimental script for different data is encouraged.

.. _dwi2mask_config:

Configuration file options
--------------------------

There are many options that can be set within the *MRtrix3*
:ref:`mrtrix_config` that directly influence the operation of the ``dwi2mask``
command. These are included in the :ref:`config_file_options` page, but are
mentioned here also for discoverability:

-  ``Dwi2maskAlgorithm``

   For those :ref:`dwi2mask_python`, this is the ``dwi2mask`` algorithm
   that will be invoked. If not explicitly set, the ``legacy`` algorithm
   will be used.

   .. NOTE::

       Setting this configuration file option does *not* enable the
       utilisation of ``dwi2mask`` without manually specifying the
       algorithm to be used. For manual usage, the algorithm must *always*
       be specified. This option *only* controls the algorithm that will
       be used when ``dwi2mask`` is invoked from inside one of the Python
       scripts provided with *MRtrix3*.

-  ``Dwi2maskTemplateSoftware``

   If ``dwi2mask b02template`` is invoked, and the ``-software`` command-line
   option is *not* used, the value of this option determines the software
   tool that will be utilised for registration to the template and
   back-propagation of the mask in template space to the subject's DWI
   data. In the absence of this configuration file option, ``antsquick``
   (i.e. ANTs_ ``antsRegistrationSyNQuick.sh``) will be used.

-  ``Dwi2maskTemplateImage`` and ``Dwi2maskTemplateMask``

   This pair of configuration file options allow the user to pre-specify the
   filesystem locations of the two images (T2-weighted template and
   corresponding binary mask) to be utilised by the ``dwi2mask ants`` and
   ``dwi2mask b02template`` algorithms. Note that there is no "default" template
   to be utilised by these algorithms; so the user *must* either include these
   entries in their configuration file, or manually specify the ``-template``
   command-line  option whenever they use ``dwi2mask ants`` or
   ``dwi2mask b02template``. If the value of configuration file option
   "``Dwi2maskAlgorithm``" is "``ants``" or "``b02template``", then
   these two entries *must also* be specified.

-  ``Dwi2maskTemplateANTsQuickOptions``, ``Dwi2maskTemplateANTsFullOptions``,
   ``Dwi2maskTemplateFSLFlirtOptions`` and ``Dwi2maskTemplateFSLFnirtConfig``

   These options allow full automated control over the parameters with which
   the external neuroimaging software package registration commands are
   executed. If one of the relevant ``dwi2mask b02template`` command-line options
   is used explicitly (``-ants_options``, ``-flirt_options``, ``-fnirt_config``),
   that information takes precedence; otherwise, if one of these configuration
   file entries is set, that information will be propagated directly to the
   relevant command.

.. _AFNI: https://afni.nimh.nih.gov/
.. _ANTs: http://stnava.github.io/ANTs/
.. _FSL: https://fsl.fmrib.ox.ac.uk/fsl/fslwiki
.. _HD-BET: https://github.com/MIC-DKFZ/HD-BET
.. _MRtrix3_connectome: https://github.com/BIDS-Apps/MRtrix3_connectome
.. _SynthStrip: https://surfer.nmr.mgh.harvard.edu/docs/synthstrip/
.. _flirt: https://fsl.fmrib.ox.ac.uk/fsl/fslwiki/FLIRT
.. _fnirt: https://fsl.fmrib.ox.ac.uk/fsl/fslwiki/FNIRT
.. _invwarp: https://fsl.fmrib.ox.ac.uk/fsl/fslwiki/FNIRT/UserGuide#invwarp
.. _applywarp: https://fsl.fmrib.ox.ac.uk/fsl/fslwiki/FNIRT/UserGuide#Now_what.3F_--_applywarp.21

