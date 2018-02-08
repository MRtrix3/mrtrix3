Frequently Asked Questions (FAQ)
================================

This page contains a collection of topics that are frequently raised in
discussions and on the `community forum <http://community.mrtrix.org>`__.
If you are seeking an answer to a question that specifically relates to
*MRtrix3* performance issues or crashes, please check the `relevant
documentation page <performance_and_crashes>`__.


Processing of HCP data
----------------------

We expect that a number of users will be wanting to use *MRtrix3* for the
analysis of data from the Human Connectome Project (HCP). These data do however
present some interesting challenges from a processing perspective. Here I will
try to list a few ideas, as well as issues that do not yet have a robust
solution; I hope that any users out there with experience with these data will
also be able to contribute with ideas or suggestions.

Do my tracking parameters need to be changed for HCP data?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Probably. For instance, the default parameters for length criteria are
currently set based on the voxel size rather than absolute values (so e.g.
animal data will still get sensible defaults). With such high resolution data,
these may not be appropriate. The default maximum length is 100 times the voxel
size, or only 125mm at 1.25mm isotropic; this would preclude reconstruction of
a number of long-range pathways in the brain, so should be overridden with
something more sensible. The minimum length is more difficult, but in the
absence of a better argument I'd probably stick with the default (5 x voxel
size, or 2 x voxel size if ACT is used).

Also, the default step size for iFOD2 is 0.5 times the voxel size; this will
make the track files slightly larger than normal, and will also make the tracks
slightly more jittery, but actually disperse slightly less over distance, than
standard resolution data. People are free to experiment with the relevant
tracking parameters, but we don't yet have an answer for how these things
should ideally behave.

Is it possible to use data from all shells in CSD?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The default CSD algorithm provided in the :ref:`dwi2fod` command is only
compatible with a single b-value shell, and will by default select the shell
with the largest b-value for processing.

The `Multi-Shell Multi-Tissue (MSMT) CSD
<http://www.sciencedirect.com/science/article/pii/S1053811914006442>`__ method
has now been incorporated into *MRtrix3*, and is provided as part of the
:ref:`dwi2fod` command. There are also instructions for its use provided in
the `documentation <multi_shell_multi_tissue_csd>`__.

The image data include information on gradient non-linearities. Can I make use of this?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Again, unfortunately not yet. Making CSD compatible with such data is more
difficult than other diffusion models, due to the canonical response function
assumption. To me, there are two possible ways that this could be handled:

- Use the acquired diffusion data to interpolate / extrapolate predicted data
  on a fixed b-value shell.

- Generate a representation of the response function that can be interpolated
  / extrapolated as a function of b-value, and therefore choose an appropriate
  response function per voxel.

Work is underway to solve these issues, but there's nothing available yet. For
those wanting to pursue their own solution, bear in mind that the gradient
non-linearities will affect both the effective b-value *and* the effective
diffusion sensitisation directions in each voxel.  Otherwise, the FODs look
entirely reasonable without these corrections...

The anatomical tissue segmentation for ACT from :ref:`5ttgen` ``fsl`` seems even worse than for 'normal' data...?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The combination of high spatial resolution and high receiver coil density
results in a pretty high noise level in the middle of the brain.  This in turn
can trick an intensity-based segmentation like FSL's FAST into mislabeling
things; it just doesn't have the prior information necessary to disentangle
what's in there. I haven't looked into this in great detail, but I would very
much like to hear if users have discovered more optimal parameters for FAST, or
alternative segmentation software, for which they have been impressed by the
results.


Generating Track-weighted Functional Connectivity (TW-FC) maps
--------------------------------------------------------------

This example demonstrates how these maps were derived, *precisely* as performed
in the `relevant NeuroImage paper
<http://www.sciencedirect.com/science/article/pii/S1053811912012402>`__.
Assumes that you have a whole-brain tractogram named ``tracks.tck``, and a 3D
volume named ``FC_map.mif`` representing an extracted FC map with appropriate
thresholding.

Initial TWI generation:

.. code-block:: console

    $ tckmap tracks.tck temp.mif <-template / -vox options> -contrast scalar_map -image FC_map.mif -stat_vox mean -stat_tck sum

Deriving the mask (voxels with at least 5 streamlines with non-zero TW
values):

.. code-block:: console

    $ tckmap tracks.tck - -template temp.mif -contrast scalar_map_count -image FC_map.mif | mrcalc - 5 -ge mask.mif -datatype bit

Apply the mask:

.. code-block:: console

    $ mrcalc temp.mif mask.mif -mult TWFC.mif


Handling SIFT2 weights
----------------------

With the original :ref:`tcksift` command, the output is a *new track file*,
which can subsequently be used as input to any command independently of the
fact that SIFT has been applied. SIFT2 is a little trickier: the output of the
:ref:`tcksift2` command is a *text file*. This text file contains one line for
every streamline, and each line contains a number; these are the weights of the
individual streamlines.  Importantly, the track file that was used as input to
the :ref:`tcksift2` command is *unaffected* by the execution of that command.

There are therefore two important questions to arise from this:

How do I use the output from SIFT2?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Any *MRtrix3* command that receives a track file as input will also have a
command-line option, ``-tck_weights_in``. This option is used to pass the
weights text file to the command. If this option is omitted, then processing
will proceed as normal for the input track file, but without taking the weights
into consideration.

Why not just add the weight information to the track data?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``.tck`` file format was developed quite a long time ago, and doesn't have
the capability of storing such data. Therefore, combining per-streamline
weighting data with the track data itself would require either modifying this
format (which would break compatibility with MRtrix 0.2, and any other
non-MRtrix code that uses this format), using some other existing format for
track data (which, given our experiences with image formats, can be
ill-devised), or creating a new format (which would need to support a lot more
than just per-streamline weights in order to justify the effort, and would
likely become a fairly lengthy endeavour).

Furthermore, writing to such a format would require duplicating all of the raw
track data from the input file into a new output file. This is expensive in
terms of time and HDD space; the original file could be deleted afterwards, but
it would then be difficult to perform any operations on the track data where
the streamline weight information should be ignored (sure, you could have a
command-line option to ignore the weights, but is that any better than having a
command-line option to input the weights?)

So, for now, it is best to think of the weights file provided by
:ref:`tcksift2` as *accompanying* the track file, containing additional data
that must be *explicitly* provided to any commands in order to be used.  The
track file can also be used *without* taking into account the streamline
weights, simply by *not* providing the weights.


Making use of Python scripts library
------------------------------------

In addition to the principal binary commands, *MRtrix3* also includes a number
of Pyton scripts for performing common image processing tasks.  These make use
of a relatively simple set of library functions that provide a certain leven of
convenience and consistency for building such scripts (e.g. common format help
page; command-line parsing; creation, use and deletion of temporary working
directory; control over command-line verbosity).

It is hoped that in addition to growing in complexity and capability over time,
this library may also be of assistance to users when building their own
processing scripts, rather than the use of e.g. Bash. The same syntax as that
used in the provided scripts can be used. If however the user wishes to run a
script that is based on this library, but is *not* located within the *MRtrix3*
``scripts/`` directory, it is necessary to explicitly inform Python of the
location of those libraries; e.g.:

.. code-block:: console

    $ export PYTHONPATH=/home/user/mrtrix3/lib:$PYTHONPATH
    $ ./my_script [arguments] (options)

(Replace the path to the *MRtrix3* "lib" directory with the location of your
own installation)


``tck2connectome`` no longer has the ``-contrast X`` option...?
-------------------------------------------------------------------------

The functionalities previously provided by the ``-contrast`` option in this
command can still be achieved, but through more explicit steps:

``tck2connectome -contrast mean_scalar``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: console

    $ tcksample tracks.tck scalar.mif mean_scalars.csv -stat_tck mean
    $ tck2connectome tracks.tck nodes.mif connectome.csv -scale_file mean_scalars.csv -stat_edge mean

The first step samples the image ``scalar.mif`` along each streamline,
calculates the *mean sampled value along each streamline*, and stores these
values into file ``mean_scalars.csv`` (one value for every streamline). The
second step then assigns the value associated with each streamline during
connectome construction to be the values from this file, and finally calculates
the value of each edge to be the *mean of the values for the streamlines in
that edge*.

``tck2connectome -contrast meanlength``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: console

    $ tck2connectome tracks.tck nodes.mif connectome.csv -scale_length -stat_edge mean

For each streamline, the contribution of that streamline to the relevant edge
is *scaled by the length* of that streamline; so, in the absence of any other
scaling, the contribution of that streamline will be equal to the length of the
streamline in mm. Finally, for each edge, take the *mean* of the values
contributed from all streamlines belonging to that edge.

``tck2connectome -contrast invlength_invnodevolume``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: console

    $ tck2connectome tracks.tck nodes.mif connectome.csv -scale_invlength -scale_invnodevol

Rather than acting as a single 'contrast', scaling the contribution of each
streamline to the connectome by *both* the inverse of the streamline length
*and* the inverse of the sum of node volumes is now handled using two
separate command-line options. The behaviour is however identical to the
old usage.


Visualising streamlines terminations
------------------------------------

I am frequently asked about Figures 5-7 in the `Anatomically-Constrained
Tractography <http://www.sciencedirect.com/science/article/pii/S1053811912005824>`__
article, which demonstrate the effects that the ACT method has on the
locations of streamlines terminations. There are two different techniques
used in these figures, which I'll explain here in full.

-  Figure 6 shows *streamlines termination density maps*: these are 3D maps
   where the intensity in each voxel reflects the number of streamlines
   terminations within that voxel. So they're a bit like Track Density Images
   (TDIs), except that it's only the streamlines termination points that
   contribute to the map, rather than the entire streamline. The easiest way to
   achieve this approach is with the ``tckmap`` command, using the
   ``-ends_only`` option.

-  Figures 5 and 7 display large dots at the streamline endpoints lying within
   the displayed slab, in conjunction with the streamlines themselves and a
   background image. Unfortunately this functionality is not yet
   implemented within *MRtrix3*, so duplicating this type of visualisation
   requires a bit of manual manipulation and software gymnastics:

   -  Use the new ``tckresample`` command, with the ``-endpoints`` option,
      to generate a new track file that contains only the two endpoints of
      each streamline.

   -  Load this track file into the *old MRtrix 0.2 version of ``mrview``*.
      This software can be acquired `here <https://github.com/jdtournier/mrtrix-0.2>`__.
      Note that you will likely want to *not* run the installation component
      of the build for this software; that way you should not encounter
      issues with conflicting commmand names between MRtrix versions. This
      does however mean that you will need to provide the full path to the
      MRtrix 0.2 ``mrview`` executable in order to run it.

   -  Within the ``mrview`` tractography tool, enable the 'depth blend'
      option. This will display each streamline point as a dot, rather than
      drawing lines between the streamline points.

   -  Adjust the brightness / contrast of the background image so that it is
      completely black.

   -  Take a screenshot.

   -  Remove the streamline endpoints track file from the tractography tool,
      and disable the 'depth blend' option (it's best to disable the 'depth
      blend' option before opening any larger track file).

   -  Reset the windowing of the main image, and/or load the complete tracks
      file, and take an additional screenshot, making sure not to move the
      view focus or resize the ``mrview`` window (so that the two screenshots
      overlay on top of one another).

   -  The two screenshots are then combined using image editing software such
      as GIMP. The colors of the termination points can also be modified
      independently before they are combined with the second screenshot. One
      trick I used in this manuscript was to rotate the hue of the termination
      screenshot by 180 degrees: this provides a pseudo-random coloring of the
      termination points that contrasts well against the tracks.


Unusual result following use of ``tcknormalise``
------------------------------------------------

Sometimes, following the use of the ``tcknormalise`` command, an unusual
effect may be observed where although the bulk of the streamlines may be
aligned correctly with the target volume / space, a subset of streamlines
appear to converge very 'sharply' toward a particular point in space.

This is caused by the presence of zero-filling in the non-linear warp
field image. In some softwares, voxels for which a proper non-linear
transformation cannot be determined between the two images will be filled
with zero values. However, ``tcknormalise`` will interpret these values as
representing an intended warp for the streamlines, such that streamline
points within those voxels will be spatially transformed to the point
[0, 0, 0] in space - this results in the convergence of many streamlines
toward the singularity point.

The solutioin is to use the ``warpcorrect`` command, which identifies voxels
that contain the warp [0, 0, 0] and replaces them with [NaN, NaN, NaN]
("NaN" = "Not a Number"). This causes ``tcknormalise`` to _discard_ those
streamline points; consistently with the results of registration, where
appropriate non-linear transformation of these points could not be determined.


Encountering errors using ``5ttgen fsl``
----------------------------------------

The following error messages have frequently been observed from the
``5ttgen fsl`` script:

.. code-block:: console

    FSL FIRST has failed; not all structures were segmented successfully
    Waiting for creation of new file "first-L_Accu_first.vtk"
    FSL FIRST job has been submitted to SGE; awaiting completion
      (note however that FIRST may fail silently, and hence this script may hang indefinitely)

Error messages that may be found in the log files within the script's
temporary directory include:

.. code-block:: console

    Cannot open volume first-L_Accu_corr for reading!
    Image Exception : #22 :: ERROR: Could not open image first_all_none_firstseg
    WARNING: NO INTERIOR VOXELS TO ESTIMATE MODE
    vector::_M_range_check
    terminate called after throwing an instance of 'RBD_COMMON::BaseException'
    /bin/sh: line 1:  6404 Aborted                 /usr/local/packages/fsl-5.0.1/bin/fslmerge -t first_all_none_firstseg first-L_Accu_corr first-R_Accu_corr first-L_Caud_corr first-R_Caud_corr first-L_Pall_corr first-R_Pall_corr first-L_Puta_corr first-R_Puta_corr first-L_Thal_corr first-R_Thal_corr

These various messages all relate to the fact that this script makes use of
FSL's FIRST tool to explicitly segment sub-cortical grey matter structures,
but this segmentation process is not successful in all circumstances.
Moreover, there are particular details with regards to the implementation of
the FIRST tool that make it awkward for the `5ttgen fsl`` script to invoke
this tool and appropriately detect whether or not the segmentation was
successful.

It appears as though a primary source of this issue is the use of FSL's
``flirt`` tool to register the T1 image to the DWIs before running
``5ttgen fsl``. While this is consistent with the recommentation in the
:ref:`act` documentation, there is an unintended consequence of performing
this registration step specifically with the ``flirt`` tool prior to
``5ttgen fsl``. With default usage, ``flirt`` will not only _register_ the
T1 image to the DWIs, but also _resample_ the T1 to the voxel grid of the
DWIs, greatly reducing its spatial resolution. This may have a concomitant
effect during the sub-cortical segmentation by FIRST: The voxel grid is
so coarse that it is impossible to find any voxels that are entirely
encapsulated by the surface corresponding to the segmented structure,
resulting in an error within the FIRST script.

If this is the case, it is highly recommended that the T1 image *not* be
resampled to the DWI voxel grid following registration; not only for the
issue mentioned above, but also because ACT is explicitly designed to take
full advantage of the higher spatial resolution of the T1 image. If
``flirt`` is still to be used for registration, the solution is to instruct
``flirt`` to provide a *transformation matrix*, rather than a translated &
resampled image:

.. code-block: console

    $ flirt -in T1.nii -ref DWI.nii -omat T12DWI_flirt.mat -dof 6

That transformation matrix should then applied to the T1 image in a manner
that only influences the transformation stored within the image header, and
does *not* resample the image to a new voxel grid:

.. code-block: console

    $ transformconvert T12DWI_flirt.mat T1.nii DWI.nii flirt_import T12DWI_mrtrix.txt
    $ mrtransform T1.nii T1_registered.mif -linear T12DWI_mrtrix.txt

If the T1 image provided to ``5ttgen fsl`` has _not_ been erroneously
down-sampled, but issues are still encountered with the FIRST step, another
possible solution is to first obtain an accurate brain extraction, and then
run ``5ttgen fsl`` using the ``--premasked`` option. This results in the
registration step of FIRST being performed based on a brain-extracted
template image, which in some cases may make the process more robust.

For any further issues, the only remaining recommendations are:

-  Investigate the temporary files that are generated within the script's
   temporary directory, particularly the FIRST log files, and search for
   any indication of the cause of failure.

-  Try running the FSL ``run_first_all`` script directly on your original
   T1 image. If this works, then further investigation could be used to
   determine precisely which images can be successfully segmented and
   which cannot. If it does not, then it may be necessary to experiment
   with the command-line options available in the ``run_first_all`` script.


How do I use atlas / parcellation "X"?
--------------------------------------

Whether dealing with individual subject data, or a population-specific
template, it can be desirable to obtain spatial correspondence between
your own data and some other atlas image. This includes taking a
parcellation that is defined in the space of that atlas and transforming
it onto the subject / template image.

Our recommended steps for achieving this are:

1. Perform registration from image of interest to target atlas

   -  Since this registration is not always intra-modal, and image
      intensities may vary significantly, here we recommend using FSL
      ``flirt``.

   -  12 degrees of freedom affine registration is performed to account
      for gross differences in brain shape.

   -  ``flirt`` must be explicitly instructed to provide a transformation
      *matrix*, rather than a transformed & re-gridded image.

   .. code-block:: console

      flirt -in my_image.mif -ref target_atlas.mif -omat image2atlas_flirt.mat -dof 12

2. Convert the transformation matrix estimated by ``flirt`` into
   *MRtrix3* convention

   .. code-block:: console

      transformconvert image2atlas_flirt.mat my_image.mif target_atlas.mif flirt_import image2atlas_mrtrix.txt

3. Invert the transformation matrix to obtain the transformation from atlas
   space to your image

   .. code-block:: console

      transformcalc image2atlas_mrtrix.txt invert atlas2image_mrtrix.txt

4. Apply this transformation to the parcellation image associated with
   the atlas

   -  Due to the use of a full affine registration (12 degrees of freedom)
      rather than a rigid-body registration (6 degrees of freedom), it is
      preferable to re-sample the parcellation image to the target image
      voxel grid, rather than altering the image header transformation
      only.

   -  When re-sampling a parcellation image to a different image grid,
      nearest-neighbour interpolation must be used; otherwise the underlying
      integer values that correspond to parcel identification indices will
      be lost.

   .. code-block:: console

      mrtransform target_parcellation.mif -linear atlas2image_mrtrix.txt -template my_image.mif parcellation_in_my_image_space.mif
