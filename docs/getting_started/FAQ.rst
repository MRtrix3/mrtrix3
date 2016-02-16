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

Not yet. We are working on inclusion of `Multi-Shell Multi-Tissue (MSMT)
CSD <http://www.sciencedirect.com/science/article/pii/S1053811914006442>`__
into MRtrix3, and hopefully it will be there soon. For now, any command
in MRtrix that operates on diffusion data will by default automatically
select the largest b-value shell for processing.

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

The anatomical tissue segmentation for ACT from ``act_anat_prepare_fsl`` seems even worse than for 'normal' data...?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

Including spinal projections in connectome
------------------------------------------


