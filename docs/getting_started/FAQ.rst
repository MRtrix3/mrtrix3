Troubleshooting
=====

.. _remote_display:

Remote display issues
---------

The GUI components in MRtrix3 (``mrview`` & ``shview``) use the OpenGL
3.3 API to make full use of modern graphics cards. Unfortunately, X11
forwarding is not supported for OpenGL >= 3. There are a number of
reasons for this:

-  OpenGL 1 & 2 used the `OpenGL fixed function
   pipeline <https://www.opengl.org/wiki/Fixed_Function_Pipeline>`__
   (now deprecated), whereas OpenGL >= 3 relies much more explicitly on
   `shaders <https://www.opengl.org/wiki/Shader>`__ and `buffer
   objects <https://www.opengl.org/wiki/Buffer_Object>`__. Amongst other
   things, the use of buffer objects implies that potentially very large
   amounts of data be downloaded onto the GPU. In a X11 forwarding
   context, this would mean transferring these data over the network,
   which would probably end up being prohibitively slow in a sufficient
   number of situations that including support for it into the
   `GLX <http://en.wikipedia.org/wiki/GLX>`__ was not thought to be
   worth the effort.

-  X11 is unbelievably outdated, even according to the
   `X.org <http://www.x.org/wiki/>`__ developers themselves (as very
   clearly explained in this `linux.conf.au
   talk <https://www.youtube.com/watch?v=RIctzAQOe44>`__). Current
   development efforts are going into its replacement,
   `Wayland <http://wayland.freedesktop.org/>`__, which will start
   replacing X11 in earnest over the next few years (it's already
   available and usable on the latest distributions). Thankfully, remote
   display capability is planned for Wayland, and `support for it has
   already been
   added <http://www.phoronix.com/scan.php?page=news_item&px=MTM0MDg>`__.

So it is not possible to use ``mrview`` or ``shview`` over a standard
remote X11 connection.

Why does MRtrix3 use OpenGL 3.3 if it come with such limitations?
^^^^^^^^^^^^^^^^^^^

Because it's clearly the most future-proof option. The `older OpenGL
versions are
deprecated <https://www.opengl.org/wiki/Fixed_Function_Pipeline>`__, and
not recommended for modern applications. The OpenGL 3.3 API is much
closer to the way modern graphics hardware works, and can therefore
provide better performance. Finally, as explained above, X11 will
eventually be phased out anyway...

What can be done about this?
^^^^^^^^^^^^^^^^^^^

There are a number of options available to deal with this, each with
their own idiosyncraties. The simplest is to render locally (option 1),
the other options require a fair bit of setting up on the server, and
potentially also on the clients.

1. Use MRView locally
"""""""""""""""""""

This is the simplest option, and allows the use of the local graphics
hardware (much like X11 forwarding would have). To use this relatively
seamlessly, the simplest option is to access the remote data using a
network filesystem, such as
`SSHFS <http://en.wikipedia.org/wiki/SSHFS>`__,
`SMB <http://en.wikipedia.org/wiki/Server_Message_Block>`__ or
`NFS <http://en.wikipedia.org/wiki/Network_File_System>`__, and run
MRView locally, loading the data from the network share. While this may
seem inefficient, bear in mind that MRtrix3 will typically only load the
data it needs to, so operation will probably not be slower than it would
have been with the MRtrix 0.2.x version. Besides, the largest data files
are likely to be track files (which will need to be loaded in their
entirety); in the MRtrix 0.2.x version these needed to be streamed in
whole over the network *for every screen update*.

Of the networked filesystems listed above, the simplest to use would
probably be `SSHFS <http://en.wikipedia.org/wiki/SSHFS>`__, since it
shouldn't require any additional setup on the server (assuming users
already have an SSH account), and is readily available on all platforms
(using `Win-SSHFS <https://code.google.com/p/win-sshfs/>`__ on Windows,
`OSXFuse <http://osxfuse.github.io/>`__ on MacOSX).

2. Use an OpenGL-capable VNC server
""""""""""""""""""""""""""""""""""

Using the `VNC
protocol <http://en.wikipedia.org/wiki/Virtual_Network_Computing>`__,
the server is responsible for doing all the rendering remotely, and
sends the resulting screen updates over the network. With this approach,
users are presented with a full-blown desktop environment running on the
server. This may consume too many resources on the remote server,
depending on the desktop environment used. Also, since rendering is
performed on the remote server, it needs to be equipped with an OpenGL
3.3 capable graphics stack - this means decent hardware *and* an up to
date driver. However, it has the advantage of being widely supported and
readily available on all platforms, with many implementations available.
The only tricky part here is ensuring the VNC server is OpenGL-capable.
As far as I can tell, `x11vnc <http://www.karlrunge.com/x11vnc/>`__ can
be used for this.

3. Use VirtualGL to allow OpenGL forwarding within X11
""""""""""""""""""""""""""""""""""""""""""""""""""""""

The `VirtualGL project <http://www.virtualgl.org/>`__ offers a means of
rendering OpenGL graphics on the remote server, and sending the updated
contents of the OpenGL window to the local display, alongside the normal
X11 connection. This provides a means of running ``mrview`` in a
potentially more familiar X11 over SSH session. As with the VNC
solution, rendering needs to be performed on the remote server, meaning
it needs to be equipped with an OpenGL 3.3 capable graphics stack - this
means decent hardware *and* an up to date driver. Also, it requires the
installation of additional software on the local system. Finally, for
this to work, all OpenGL commands need to be prefixed with ``vglrun``
(not particularly problematic as this can be scripted or aliased). This
has been reported to work well with MRtrix3.


Unusual symbols on terminal
---------------------------

When running MRtrix commands on certain terminal emulators, you may see
unusual characters appearing in the terminal output, that look something
like the following:

.. code::

    $ mrinfo fa.mif -debug
    mrinfo: ←[00;32m[INFO] opening image "fa.mif"...←[0m
    mrinfo: ←[00;34m[DEBUG] reading key/value file "fa.mif"...←[0m
    mrinfo: ←[01;31m[ERROR] failed to open key/value file "fa.mif": No such file or directory←[0m

MRtrix uses VT100 terminal control codes to add colour to the terminal
output, and to clear the terminal line of text when updating the text
displayed during certain processes. Some terminal emulators may not
have support for these codes, in which case unwanted characters and
symbols may instead be displayed.

There are two possible solutions:

1. Use a different terminal emulator. In particular, earlier instructions
for installing MRtrix3 on Windows involved the use of the terminal provided
with Git for Windows; this is known to not support VT100 codes. The
current recommendation for `MRtrix3 Windows installation <windows-install>`__
is based on
`MSYS2 <http://sourceforge.net/p/msys2/wiki/MSYS2%20introduction/>`__;
the **'MinGW-w64 Win64 Shell'** provided in this installation is known to
support VT100 codes.

2. Terminal colouring can be disabled using the MRtrix
`configuration file <config>`. Add the following line to either the
system-wide or user config file to disable these advanced terminal features:

.. code::

    TerminalColor: 0


Processing of HCP data
------------------------

We expect that a number of users will be wanting to use MRtrix3 for the
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

The default CSD algorithm provided in the ``dwi2fod`` command is only
compatible with a single b-value shell, and will by default select the
shell with the largest b-value for processing.

The `Multi-Shell Multi-Tissue (MSMT)
CSD <http://www.sciencedirect.com/science/article/pii/S1053811914006442>`__
method has now been incorporated into MRtrix3, and is provided as the
``msdwi2fod`` command. There are also instructions for its use provided
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

The anatomical tissue segmentation for ACT from ``5ttgen fsl`` seems even worse than for 'normal' data...?
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

The main memory requirement for SIFT is that for every streamline, it
must store a list of every `fixel <Dixels-and-Fixels>`__ traversed, with
an associated streamline length through each voxel. With a spatial
resolution approximately double that of 'standard' DWI, the number of
unique fixels traversed by each streamline will go up by a factor of
around 3, with a corresponding increase in RAM usage. There is literally
nothing I can do to reduce the RAM usage of SIFT; it's fully optimised.

One thing you can do however, is just down-scale the FOD image prior to
running SIFT: ``mrresize in.mif out.mif -scale 0.5 -interp sinc``. This
will reduce the RAM usage to more manageable levels, and realistically
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
------------------------------------------

With the original ``tcksift`` command, the output is a _new track file_,
which can subsequently be used as input to any command independently of
the fact that SIFT has been applied. SIFT2 is a little trickier: the
output of the ``tcksift2`` command is a _text file_. This text file
contains one line for every streamline, and each line contains
a number; these are the weights of the individual streamlines.
Importantly, the track file that was used as input to the ``tcksift2``
command is *unaffected* by the execution of that command.

There are therefore two important questions to arise from this:

How do I use the output from SIFT2?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Any MRtrix3 command that receives a track file as input will also have
a command-line option, `-tck_weights_in`. This option is used to pass
the weights text file to the command. If this option is omitted, then
processing will proceed as normal for the input track file, but without
taking the weights into consideration.

Why not just add the weight information to the track data?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The `.tck` file format was developed quite a long time ago, and doesn't
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
``tcksift2`` as *accompanying* the track file, containing additional data
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
