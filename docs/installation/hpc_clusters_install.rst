=============
HPC clusters installation
=============

These instructions outline a few issues specific to high-performance
computing (HPC) systems.

Installing *MRtrix3*
------------------

Most HPC clusters will run some flavour of GNU/Linux and hence
a cluster administrator should be able to follow the steps outlined for a :ref:`linux_install`. 
In particular, if your sysadmin is able to install the required dependencies (the
preferred option), you should be able to subsequently :ref:`linux_build`.

However, it is not uncommon for HPC systems to run stable, and hence
relatively old distributions, with outdated dependencies. This is
particularly problematic since *MRtrix3* relies on recent technologies
(C++11, OpenGL 3.3), which are only available on recent distributions.
There is therefore a good chance these dependencies simply cannot be
installed (certainly not without a huge amount of effort on the part of
your sysadmin). In such cases, one can instead attempt a :ref:`linux_standalone`.

Remote display
--------------

Most people would expect to be able to run ``mrview`` on the server using
X11 forwarding. Unfortunately, this will not work without some effort -
please refer to :ref:`remote_display` for details.

Configuration
-------------

There are a number of parameters that can be set in the configuration
file that are highly relevant in a HPC environment, particularly when
the user's home folder is stored over a network-based filesystem (as is
often the case). The *MRtrix3* configuration file is located either
system-wide in ``/etc/mrtrix.conf``, and/or in each user's home folder
in ``~/.mrtrix.conf``. Entries consist of ``key: value`` entries, one
per line, stored as ASCII text.

-  **NumberOfThreads** (default: `hardware
   concurrency <http://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency>`__,
   as reported by the system): by default, *MRtrix3* will use as many
   threads as the system reports being able to run concurrently. You may
   want to change that number to a lower value, to prevent *MRtrix3* from
   taking over the system entirely. This is particularly true if you
   anticipate many users running many *MRtrix3* commands concurrently.

-  **TmpFileDir** (default: '/tmp'): any image data passed from one
   *MRtrix3* command to the next using a Unix pipeline is actually stored
   in a temporary file, and its filename passed to the next command.
   While this is fine if the filesystem holding the temporary file is
   locally backed and large enough, it can cause significant slowdown
   and bottlenecks if it resides on a networked filesystems, as the
   temporary file will most likely need to be transferred in its
   entirety over the network and back again. Also, if the filesystem is
   too small, *MRtrix3* commands may abort when processing large files. In
   general, the ``/tmp`` folder is likely to be the most appropriate
   (especially if mounted as
   `tmpfs <http://en.wikipedia.org/wiki/Tmpfs>`__). If however it is not
   locally mounted, or too small, you may want to set this folder to
   some other more suitable location.

-  **TrackWriterBufferSize** (default: 16777216). When writing out track
   files, *MRtrix3* will buffer up the output and write out in chunks of
   16MB, to limit the frequency of write() calls and the amount of IO
   requests. More importantly, when several instances of *MRtrix3* are
   generating tracks concurrently and writing to the same filesystem,
   frequent small writes will result in massive fragmentation of the
   output files. By setting a large buffer size, the chances of writes
   being concurrent is reduced drastically, and the output files are
   much less likely to be badly fragmented. Note that fragmentation can
   seriously affect the performance of subsequent commands that need to
   read affected data. Depending on the type of operations performed, it
   may be beneficial to use larger buffer sizes, for example 256MB. Note
   that larger numbers imply greater RAM usage to hold the data prior to
   write-out, so it is best to keep this much smaller than the total RAM
   capacity.


