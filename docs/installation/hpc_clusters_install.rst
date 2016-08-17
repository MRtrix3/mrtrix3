=============
HPC clusters installation
=============

These instructions outline a few issues specific to high-performance
computing (HPC) systems.

Installing *MRtrix3*
--------------------

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
Alternatively, if you (and your sysadmin) are comfortable with installation
of dependencies from source within your home directory, you can try the
instructions below.

Installation of *MRtrix3* and dependencies from source
------------------------------------------------------

The following instructions list the steps I used to compile *MRtrix3*
natively on a local HPC cluster. Replicating these instructions line-for-line
may not work on another system; I'm just providing these instructions here
in case they help to point somebody in the right direction, or encourage users
to try a native installation rather than resorting to transferring binaries
compiled on another system.

-  Installing a C++11-compliant g++ from source

   Note that during this process, there will be *three* ``gcc`` directories
   created: one is for the source code (including that of some prerequisites),
   one is for compilation objects, and one is the target of the final
   installation (since you almost certainly won't be able to install this
   version of ``gcc`` over the top of whatever is provided by the HPC
   sysadmin).
   
   ::

       svn co svn://gcc.gnu.org/svn/gcc/branches/gcc-5-branch gcc_src/
   
   (*Don't* checkout the trunk ``gcc`` code; *MRtrix3* will currently not compile with it)
   
   The following ``gcc`` dependencies will be built as part of the ``gcc``
   compilation, provided that they are placed in the `correct location <https://gcc.gnu.org/install/prerequisites.html>`__
   within the ``gcc`` source directory.
   
   ::
   
       wget https://gmplib.org/download/gmp/gmp-6.1.1.tar.bz2
       tar -xf gmp-6.1.1.tar.bz2
       mv gmp-6.1.1/ gcc_src/gmp/
       wget ftp://ftp.gnu.org/gnu/mpc/mpc-1.0.3.tar.gz
       tar -xf mpc-1.0.3.tar.gz
       mv mpc-1.0.3/ gcc_src/mpc/
       wget http://www.mpfr.org/mpfr-current/mpfr-3.1.4.tar.gz
       tar -xf mpfr-3.1.4.tar.gz
       mv mpfr-3.1.4/ gcc_src/mpfr/
       
   With the following, the ``configure`` script (which resides within the
   ``gcc_src`` directory in this example) must *not* be executed within that
   directory; rather, it must be executed from an alternative directory, which
   will form the target location for the compilation object files. The target
   installation directory (set using the ``--prefix`` option below) must be a
   location for which you have write access; most likely somewhere in your
   home directory.
   
   ::
   
       mkdir gcc_obj; cd gcc_obj/
       ../gcc_src/configure --prefix=/path/to/installed/gcc --disable-multilib
       make && make install

-  Installing Python3 from source

   My local HPC cluster provided Python version 2.6.6, which was not adequate
   to successfully run the ``configure`` and ``build`` scripts in *MRtrix3*.
   Therefore this necessitated a manual Python install - a newer version of
   Python 2 would also work, but downloading Python 3 should result in less
   ambiguity about which version is being run.
   
   ::
   
       wget https://www.python.org/ftp/python/3.5.2/Python-3.5.2.tgz
       tar -xf Python-3.5.2.tgz
       mv Python-3.5.2/ python3/
       cd python3/
       ./configure
       ./make
       cd ../

-  Installing Eigen3

   ::

       wget http://bitbucket.org/eigen/eigen/get/3.2.8.tar.gz
       tar -xf 3.2.8.tar.gz
       mv eigen* eigen3/
       
-  Installing *MRtrix3*

   Personally I prefer to install a no-GUI version of *MRtrix3* on
   high-performance computing systems, and transfer files to my local system
   if I need to view anything; so I use the ``-nogui`` flag for the
   ``configure`` script.
   
   ::
   
       git clone https://github.com/MRtrix3/mrtrix3.git
       cd mrtrix3/
       export CXX=/path/to/installed/gcc/bin/g++
       export EIGEN_CFLAGS="-isystem /path/to/eigen3/"
       export LD_LIBRARY_PATH="/path/to/installed/gcc/lib64:$LD_LIBRARY_PATH"
       ../python3/python configure -nogui
       ../python3/python build
   
   If you encounter issues when running *MRtrix3* commands that resemble
   the following:
   
   ``mrconvert: /usr/lib64/libstdc++.so.6: version `GLIBCXX_3.4.9' not found (required by mrconvert)``
   
   This indicates that the shared library of the compiler version installed on
   the cluster is being found before that of the C++11-compliant compiler
   installed manually. The ``lib64/`` directory of the manually-installed
   ``gcc`` version must appear *before* that of the version installed on the
   cluster in the ``LD_LIBRARY_PATH`` environment variable.

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


