Frequently Asked Questions (FAQ)
================================

Processing of HCP data
----------------------

We expect that a number of users will be wanting to use *MRtrix3* for the
analysis of data from the Human Connectome Project (HCP). These data do
however present some interesting challenges from a processing
perspective. Here I will try to list a few ideas, as well as issues that
do not yet have a robust solution; I hope that any users out there with
experience with these data will also be able to contribute with ideas or
suggestions.

Do my tracking parameters need to be changed for HCP data?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Probably. For instance, the default parameters for length criteria are
currently set based on the voxel size rather than absolute values (so
e.g. animal data will still get sensible defaults). With such high
resolution data, these may not be appropriate. The default maximum
length is 100 times the voxel size, or only 125mm at 1.25mm isotropic;
this would preclude reconstruction of a number of long-range pathways in
the brain, so should be overridden with something more sensible. The
minimum length is more difficult, but in the absence of a better
argument I'd probably stick with the default (5 x voxel size, or 2 x
voxel size if ACT is used).

Also, the default step size for iFOD2 is 0.5 times the voxel size; this
will make the track files slightly larger than normal, and will also
make the tracks slightly more jittery, but actually disperse slightly
less over distance, than standard resolution data. People are free to
experiment with the relevant tracking parameters, but we don't yet have
an answer for how these things should ideally behave.

Is it possible to use data from all shells in CSD?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The default CSD algorithm provided in the :ref:`dwi2fod` command is only
compatible with a single b-value shell, and will by default select the
shell with the largest b-value for processing.

The `Multi-Shell Multi-Tissue (MSMT)
CSD <http://www.sciencedirect.com/science/article/pii/S1053811914006442>`__
method has now been incorporated into *MRtrix3*, and is provided as the
:ref:`msdwi2fod` command. There are also instructions for its use provided
in the `documentation <multi_tissue_csd>`__.

The image data include information on gradient non-linearities. Can I make use of this?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Again, unfortunately not yet. Making CSD compatible with such data is
more difficult than other diffusion models, due to the canonical
response function assumption. To me, there are two possible ways that
this could be handled:

-  Use the acquired diffusion data to interpolate / extrapolate
   predicted data on a fixed b-value shell.

-  Generate a representation of the response function that can be
   interpolated / extrapolated as a function of b-value, and therefore
   choose an appropriate response function per voxel.

Work is underway to solve these issues, but there's nothing available
yet. For those wanting to pursue their own solution, bear in mind that
the gradient non-linearities will affect both the effective b-value
*and* the effective diffusion sensitisation directions in each voxel.
Otherwise, the FODs look entirely reasonable without these
corrections...

The anatomical tissue segmentation for ACT from :ref:`5ttgen fsl` seems even worse than for 'normal' data...?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The combination of high spatial resolution and high receiver coil
density results in a pretty high noise level in the middle of the brain.
This in turn can trick an intensity-based segmentation like FSL's FAST
into mislabeling things; it just doesn't have the prior information
necessary to disentangle what's in there. I haven't looked into this in
great detail, but I would very much like to hear if users have
discovered more optimal parameters for FAST, or alternative segmentation
software, for which they have been impressed by the results.

Why does SIFT crash on my system even though it's got heaps of RAM?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The main memory requirement for `SIFT <SIFT>`_ is that for every streamline,
it must store a list of every `fixel <Dixels-and-Fixels>`__ traversed, with
an associated streamline length through each voxel. With a spatial
resolution approximately double that of 'standard' DWI, the number of
unique fixels traversed by each streamline will go up by a factor of
around 3, with a corresponding increase in RAM usage. There is literally
nothing I can do to reduce the RAM usage of SIFT; it's fully optimised.

One thing you can do however, is just down-scale the FOD image prior to
running :ref:`tcksift`: ``mrresize in.mif out.mif -scale 0.5 -interp sinc``.
This will reduce the RAM usage to more manageable levels, and realistically
probably won't have that much influence on the algorithm anyway.
Importantly you can still use the high-resolution data for tracking (or
indeed anything else); it's only the SIFT step that has the high RAM
usage. And using ``mrresize`` rather than some other software to do the
downsampling will ensure that the down-sampled image is still properly
aligned with the high-resolution image in scanner space.

Generating Track-weighted Functional Connectivity (TW-FC) maps
--------------------------------------------------------------

This example demonstrates how these maps were derived, *precisely* as
performed in the `relevant NeuroImage paper <http://www.sciencedirect.com/science/article/pii/S1053811912012402>`__.
Assumes that you have a whole-brain tractogram named ``tracks.tck``, and
a 3D volume named ``FC_map.mif`` representing an extracted FC map with
appropriate thresholding.

Initial TWI generation:

.. code::

    tckmap tracks.tck temp.mif <-template / -vox options> -contrast scalar_map -image FC_map.mif -stat_vox mean -stat_tck sum

Deriving the mask (voxels with at least 5 streamlines with non-zero TW
values):

.. code::

    tckmap tracks.tck - -template temp.mif -contrast scalar_map_count -image FC_map.mif | mrcalc - 5 -ge mask.mif -datatype bit

Apply the mask:

.. code::

    mrcalc temp.mif mask.mif -mult TWFC.mif

Handling SIFT2 weights
----------------------

With the original :ref:`tcksift` command, the output is a *new track file*,
which can subsequently be used as input to any command independently of
the fact that SIFT has been applied. SIFT2 is a little trickier: the
output of the :ref:`tcksift2` command is a *text file*. This text file
contains one line for every streamline, and each line contains
a number; these are the weights of the individual streamlines.
Importantly, the track file that was used as input to the :ref:`tcksift2`
command is *unaffected* by the execution of that command.

There are therefore two important questions to arise from this:

How do I use the output from SIFT2?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Any *MRtrix3* command that receives a track file as input will also have
a command-line option, ``-tck_weights_in``. This option is used to pass
the weights text file to the command. If this option is omitted, then
processing will proceed as normal for the input track file, but without
taking the weights into consideration.

Why not just add the weight information to the track data?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``.tck`` file format was developed quite a long time ago, and doesn't
have the capability of storing such data. Therefore, combining
per-streamline weighting data with the track data itself would require
either modifying this format (which would break compatibility with
MRtrix 0.2, and any other non-MRtrix code that uses this format), using
some other existing format for track data (which, given our experiences
with image formats, can be ill-devised), or creating a new format (which
would need to support a lot more than just per-streamline weights in
order to justify the effort, and would likely become a fairly lengthy
endeavour).

Furthermore, writing to such a format would require duplicating all of
the raw track data from the input file into a new output file. This is
expensive in terms of time and HDD space; the original file could be
deleted afterwards, but it would then be difficult to perform any
operations on the track data where the streamline weight information
should be ignored (sure, you could have a command-line option to ignore
the weights, but is that any better than having a command-line option
to input the weights?)

So, for now, it is best to think of the weights file provided by
:ref:`tcksift2` as *accompanying* the track file, containing additional data
that must be *explicitly* provided to any commands in order to be used.
The track file can also be used *without* taking into account the
streamline weights, simply by *not* providing the weights.

Making use of Python scripts library
------------------------------------

In addition to the principal binary commands, *MRtrix3* also includes a
number of Pyton scripts for performing common image processing tasks.
These make use of a relatively simple set of library functions that provide
a certain leven of convenience and consistency for building such scripts
(e.g. common format help page; command-line parsing; creation, use and
deletion of temporary working directory; control over command-line
verbosity).

It is hoped that in addition to growing in complexity and capability over
time, this library may also be of assistance to users when building their own
processing scripts, rather than the use of e.g. Bash. The same syntax as that
used in the provided scripts can be used. If however the user wishes to run a
script that is based on this library, but is *not* located within the
*MRtrix3* ``scripts/`` directory, it is necessary to explicitly inform Python
of the location of those libraries; e.g.:

.. code::

    export PYTHONPATH=/home/user/mrtrix3/scripts:$PYTHONPATH
    ./my_script [arguments] (options)

(Replace the path to the *MRtrix3* scripts directory with the location of your
own installation)

``tck2connectome`` no longer has the ``-contrast mean_scalar`` option...?
-------------------------------------------------------------------------

The functionality previously provided by this command and option can now be
achieved by instead splitting the operation into two independent steps:

.. code::

    tcksample tracks.tck scalar.mif mean_scalars.csv -stat_tck mean
    tck2connectome tracks.tck nodes.mif connectome.csv -scale_file mean_scalars.csv -stat_edge mean

The first step samples the image ``scalar.mif`` along each streamline,
calculates the *mean sampled value along each streamline*, and stores these
values into file ``mean_scalars.csv`` (one value for every streamline). The
second step then assigns the value associated with each streamline during
connectome construction to be the values from this file, and finally
calculates the value of each edge to be the *mean of the values for the
streamlines in that edge*.

Maximum spherical harmonic degree ``lmax``
------------------------------------------

What determines ``lmax`` for my image data?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For any command or script operating on data in the spherical harmonic
basis, it should be possible to manually set the maximum harmonic degree
of the output using the ``-lmax`` command-line option. If this is *not*
provided, then an appropriate value will be determined automatically.

The mechanisms by which this automatic determination of ``lmax`` occurs
are as follows:

-  Determine the maximum value for ``lmax`` that is supported by the number
   of DWI volumes in the shell being processed (or the total number of
   non-*b*=0 volumes in a single-shell acquisition). This is the number of
   coefficients required to store an anitipodally-symmetric spherical
   harmonic function:

+------+------------------+
| lmax | Required volumes |
+======+==================+
|    2 | 6                |
+------+------------------+
|    4 | 15               |
+------+------------------+
|    6 | 28               |
+------+------------------+
|    8 | 45               |
+------+------------------+
|   10 | 66               |
+------+------------------+
|   12 | 91               |
+------+------------------+
|  ... | ...              |
+------+------------------+

-  If ``lmax`` exceeds 8, reduce to 8. This is primarily based on the
   findings in `this paper <http://onlinelibrary.wiley.com/doi/10.1002/nbm.3017/abstract>`__.

-  Check the condition of the transformation between DWIs and spherical
   harmonics. If the transformation is ill-conditioned (usually indicating
   that the diffusion sensitisation gradient directions are not evenly
   distributed over the sphere or half-sphere), reduce ``lmax`` until the
   transformation is well-conditioned.

   As an example: concatenating two repeats of a 30 direction acquisition
   to produce 60 volumes will *not* support an ``lmax``=8 fit: the angular
   resolution of the data set is equivalent to 30 *unique* directions, and
   so ``lmax``=6 would be selected (and this would be accompanied by a
   command-line warning to the user).

-  In the case of spherical deconvolution, the ``lmax`` selected for FOD
   estimation will also be reduced if ``lmax`` of the provided response
   function is less than that calculated as above.

Reduced ``lmax`` in particular subjects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If you find that certain subjects within a cohort have a reduced ``lmax``
compared to the rest of the cohort (usually spotted by checking the number
of coefficients in the response function), the most likely cause is
premature termination of the diffusion sequence during scanning of that
subject, resulting in a reduced number of diffusion volumes and therefore
a reduced ``lmax`` according to the table above.

Setting ``lmax`` in different applications
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The range of permissible values for ``lmax`` depends on the particular
command being used; e.g.:

-  The way that response function estimation is currently implemented, it
   is impossible to set ``lmax`` to a value higher than that supported by the
   image data. The transformation from DWI data to spherical harmonics simply
   cannot be done in such a case, as the problem is under-determined. You can
   of course set ``lmax`` to a lower value than that supported by the data.

-  In spherical deconvolution, it *is* possible to set a higher ``lmax``
   than that supported by the data - so-called *super-resolved* spherical
   deconvolution. Here, additional information is provided by the non-negativity
   constraint to make estimation of additional spherical harmonic coefficients
   possible. However this is not guaranteed: sometimes the algorithm will fail
   in particular voxels, in cases where there are an insufficient number of
   directions in which the initial FOD estimate is negative, as the problem
   remains under-determined.

-  If performing Track Orientation Density Imaging (TODI) using
   ``tckgen -tod``, then the apodized point spread functions (aPSFs) can be
   generated at any value of ``lmax``, since the angular resolution of the
   original image data is not a limiting factor here.

Visualising streamlines terminations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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
