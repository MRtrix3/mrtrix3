.. _remote_display:

Remote display issues
======================

The GUI components in *MRtrix3* (``mrview`` & ``shview``) use the OpenGL
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

Why does *MRtrix3* use OpenGL 3.3 if it come with such limitations?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Because it's clearly the most future-proof option. The `older OpenGL
versions are
deprecated <https://www.opengl.org/wiki/Fixed_Function_Pipeline>`__, and
not recommended for modern applications. The OpenGL 3.3 API is much
closer to the way modern graphics hardware works, and can therefore
provide better performance. Finally, as explained above, X11 will
eventually be phased out anyway...

What can be done about this?
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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
``mrview`` locally, loading the data from the network share. While this may
seem inefficient, bear in mind that *MRtrix3* will typically only load the
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
"""""""""""""""""""""""""""""""""""

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
has been reported to work well with *MRtrix3*.


OpenGL version 3.3 not supported
=================================

This will typically lead to ``mrview`` crashing with a message such as:

::

    mrview: [ERROR] GLSL log [vertex shader]: ERROR: version '330' is not supported

There are three main reasons for this:

1. **Attempting to run MRView using X11 forwarding.** This will not work
   without some effort, see :ref:`remote_display` for details.

2. **Your installation genuinely does not support OpenGL 3.3.** In this
   case, the solution will involve figuring out:

   -  whether your graphics hardware can support OpenGL 3.3 at all;
   -  whether your Linux distribution provides any drivers for your
      graphics hardware that can support OpenGL 3.3;
   -  if not, whether the manufacturer of your graphics hardware
      provides drivers for Linux that can be installed on your
      distribution;
   -  how to install these drivers - a process that is invariably
      distribution-specific, and beyond the scope of this document. If
      you're having serious issues with this, you should consider asking
      on the `MRtrix3 community forum <http://community.mrtrix.org/>`__,
      you will often find others have come across similar issues and can
      provide useful advice. If you do, make sure you provide as much
      information as you can (at the very least, your exact
      distribution, including which version of it, the exact model of
      your graphics hardware, and what you've tried so far).

3. **Your installation does support OpenGL 3.3, but only provides access
   to the 3.3 functionality through the _compatibility_ profile, not through the
   (default) core profile.** To see whether this is the problem,
   you only need to add the line:

   ::

       NeedOpenGLCoreProfile: 0

   to your `MRtrix configuration file <config>`__ (typically, ``~/.mrtrix.conf``). If
   it doesn't work, you're probably stuck with reason 2.


MRView runs with visual artefacts or no display
-----------------------------------------------

If you find that MRView displays with visual glitches or a blank screen,
particularly in volume render mode, and on ATI/AMD hardware, you may find that
setting::

    NeedOpenGLCoreProfile: 0

may resolve the problem.


Unusual symbols on terminal
---------------------------

When running *MRtrix3* commands on certain terminal emulators, you may see
unusual characters appearing in the terminal output, that look something
like the following:

.. code::

    $ mrinfo fa.mif -debug
    mrinfo: ←[00;32m[INFO] opening image "fa.mif"...←[0m
    mrinfo: ←[00;34m[DEBUG] reading key/value file "fa.mif"...←[0m
    mrinfo: ←[01;31m[ERROR] failed to open key/value file "fa.mif": No such file or directory←[0m

*MRtrix3* uses VT100 terminal control codes to add colour to the terminal
output, and to clear the terminal line of text when updating the text
displayed during certain processes. Some terminal emulators may not
have support for these codes, in which case unwanted characters and
symbols may instead be displayed.

There are two possible solutions:

1. Use a different terminal emulator. In particular, earlier instructions
for installing *MRtrix3* on Windows involved the use of the terminal provided
with Git for Windows; this is known to not support VT100 codes. The
current recommendation for `*MRtrix3* Windows installation <windows-install>`__
is based on
`MSYS2 <http://sourceforge.net/p/msys2/wiki/MSYS2%20introduction/>`__;
the **'MinGW-w64 Win64 Shell'** provided in this installation is known to
support VT100 codes.

2. Terminal colouring can be disabled using the MRtrix
`configuration file <config>`__. Add the following line to either the
system-wide or user config file to disable these advanced terminal features:

.. code::

    TerminalColor: 0


Hanging on network file system when writing images
--------------------------------------------------

When any *MRtrix3* command must read or write image data, there are two
primary mechanisms by which this is performed:

1. `Memory mapping <https://en.wikipedia.org/wiki/Memory-mapped_file>`_:
The operating system provides access to the contents of the file as
though it were simply a block of data in memory, without needing to
explicitly load all of the image data into RAM.

2. Preload / delayed write-back: When opening an existing image, the
entire image contents are loaded into a block of RAM. If an image is
modified, or a new image created, this occurs entirely within RAM, with
the image contents written to disk storage only at completion of the
command.

This design ensures that loading images for processing is as fast as
possible and does not incur unnecessary RAM requirements, and writing
files to disk is as efficient as possible as all data is written as a
single contiguous block.

Memory mapping will be used wherever possible. However one circumstance
where this should *not* be used is when *write access* is required for
the target file, and it is stored on a *network file system*: in this
case, the command typically slows to a crawl (e.g. progressbar stays at
0% indefinitely), as the memory-mapping implementation repeatedly
performs small data writes and attempts to keep the entire image data
synchronised.

*MRtrix3* will now *test* the type of file system that a target image is
stored on; and if it is a network-based system, it will *not* use
memory-mapping for images that may be written to. *However*, if you
experience the aforementioned slowdown in such a circumstance, it is
possible that the particular configuration you are using is not being
correctly detected or identified. If you are unfortunate enough to
encounter this issue, please report to the developers the hardware
configuration and file system type in use.


Conflicts with previous versions of Qt
--------------------------------------

If previous versions of Qt were already installed on the system, they
can sometimes conflict with the installation of *MRtrix3*. This can
manifest in many ways, but the two most obvious one are:

-  ``./configure`` reports using the older version, but ``./build``
   completes without errors. However, MRView crashes, complaining about
   OpenGL version not being sufficient.
-  ``./configure`` reports the correct version of Qt, but ``./build``
   fails with various error messages (typically related to redefined
   macros, with previous definitions elsewhere in the code).



Compiler error during build
---------------------------

If you encounter an error during the build process that resembles the following::

    ERROR: (#/#) [CC] release/cmd/command.o
    
    /usr/bin/g++-4.8 -c -std=c++11 -pthread -fPIC -march=native -I/home/user/mrtrix3/eigen -Wall -O2 -DNDEBUG -Isrc -Icmd -I./lib -Icmd cmd/command.cpp -o release/cmd/command.o
    
    failed with output
    
    g++-4.8: internal compiler error: Killed (program cc1plus)
    Please submit a full bug report,
    with preprocessed source if appropriate.
    See for instructions.


This is most typically caused by the compiler running out of RAM. This
can be solved either through installing more RAM into your system, or
by restricting the number of threads to be used during compilation::

    NUMBER_OF_PROCESSORS=1 ./build


