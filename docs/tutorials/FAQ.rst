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
in the `documentation <Multi-Tissue-CSD>`__.

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


