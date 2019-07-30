.. _linux_install:

Linux installation
==================

We outline the steps for installing *MRtrix3* on a Linux machine. Please consult 
the `MRtrix3 forum <http://community.mrtrix.org/>`__ if you encounter any
issues with the configure, build or runtime operations of *MRtrix3*.

Check requirements
------------------

To install *MRtrix3*, you will need the following:

-  a `C++11 <https://en.wikipedia.org/wiki/C%2B%2B11>`__ compliant
   compiler (GCC version >= 4.9, clang)
-  `Python <https://www.python.org/>`__ version >= 2.7
-  The `zlib <http://www.zlib.net/>`__ compression library
-  `Eigen <http://eigen.tuxfamily.org>`__ version >= 3.2 
-  `Qt <http://www.qt.io/>`__ version >= 4.7 *[GUI components only]*

and optionally:

- `libTIFF <http://www.libtiff.org/>`__ version >= 4.0 (for TIFF support)
- `FFTW <http://www.fftw.org/>`__ version >= 3.0 (for improved performance in
  certain applications, currently only ``mrdegibbs``)
  
  

.. WARNING:: 

    To run the GUI components of *MRtrix3* (``mrview`` &
    ``shview``), you will also need:

    -  an `OpenGL <https://en.wikipedia.org/wiki/OpenGL>`__ 3.3 compliant graphics card and corresponding software driver

    Note that this implies you *cannot run the GUI components over a remote
    X11 connection*, since it can't support OpenGL 3.3+ rendering - see
    :ref:`remote_display` for details.

Install Dependencies
--------------------

The installation procedure will depend on your system. Package names may
changes between distributions, and between different releases of the
same distribution. The commands below are suggestions based on what has
been reported to work in the past, but may need to be amended. See below
for hints on how to proceed in this case.

-  Ubuntu Linux (and derivatives, e.g. Linux Mint)::

       sudo apt-get install git g++ python python-numpy libeigen3-dev zlib1g-dev libqt4-opengl-dev libgl1-mesa-dev libfftw3-dev libtiff5-dev

-  RPM-based distros (Fedora, CentOS)::

       sudo yum install git g++ python numpy eigen3-devel zlib-devel libqt4-devel libgl1-mesa-dev fftw-devel libtiff-devel

   on Fedora 24, this is reported to work::

           sudo yum install git gcc-c++ python numpy eigen3-devel zlib-devel qt-devel mesa-libGL-devel fftw-devel libtiff-devel


-  Arch Linux::

       sudo pacman -Syu git python python-numpy gcc zlib eigen qt5-svg fftw libtiff

If this doesn't work
^^^^^^^^^^^^^^^^^^^^

You may find that your package installer is unable to find the packages
listed, or that the subsequent steps fail due to missing dependencies
(particularly the ``./configure`` command). In this case, you will need
to search the package database and find the correct names for these
packages:

-  git

-  your compiler (gcc 4.9 or above, or clang)

-  Python version >2.7

-  NumPy

-  the zlib compression library and its corresponding development
   header/include files

-  the Eigen template library (only consists of development header/include files);

-  Qt version >4.7, its corresponding development header/include files,
   and the executables required to compile the code. Note this will most
   likely be broken up into several packages, depending on how your
   distribution has chosen to distribute this. You will need to get
   those that provide these Qt modules: Core, GUI, OpenGL, SVG, and the
   qmake, rcc & moc executables (note these will probably be included in
   one of the other packages).

.. WARNING::  

    The compiler included in Ubuntu 12.04 and other older distributions is no
    longer capable of compiling *MRtrix3*, as it now requires C++11 support.
    The solution is to use a newer compiler as provided by the `Ubuntu
    toolchain PPA
    <https://launchpad.net/~ubuntu-toolchain-r/+archive/ubuntu/test>`__ -
    follow the link for instructions on how to add the PPA. Once the PPA has
    been added, you'll need to issue a ``sudo apt-get update``, followed by
    ``sudo apt-get install g++-4.9``. You will probably also need to tell
    ``./configure`` to use this compiler (see ``./configure -help`` for further
    options)::

        CXX=g++-4.9 ./configure

If this *really* doesn't work
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If for whatever reasons you need to install *MRtrix3* on a system with
older dependencies, and you are unable to update the software (e.g. you
want to run *MRtrix3* on a centrally-managed HPC cluster), you can as a
last resort use the procedure described in :ref:`linux_standalone`.

Git setup
---------

If you intend to contribute to the development of *MRtrix3*, set up your git
environment as per the `Git instructions page
<https://help.github.com/articles/set-up-git/#setting-up-git>`__

.. _linux_build:

Build *MRtrix3*
---------------

1. Clone the *MRtrix3* repository::

       git clone https://github.com/MRtrix3/mrtrix3.git

   or if you have set up your SSH keys (for contributors)::

       git clone git@github.com:MRtrix3/mrtrix3.git

2. Configure the *MRtrix3* install::

       cd mrtrix3
       ./configure

   If this does not work, examine the 'configure.log' file that is
   generated by this step, it may give clues as to what went wrong.

3. Build the binaries::

       ./build

Set up *MRtrix3*
----------------

1. Update the shell startup file, so that the locations of *MRtrix3* commands
   and scripts will be added to your ``PATH`` envionment variable.
   
   If you are not familiar or comfortable with modification of shell files,
   *MRtrix3* now provides a convenience script that will perform this setup
   for you (assuming that you are using ``bash`` or equivalent interpreter).
   From the top level *MRtrix3* directory, run the following::
   
       ./set_path

2. Close the terminal and start another one to ensure the startup file
   is read (or just type 'bash')

3. Type ``mrview`` to check that everything works

4. You may also want to have a look through the :ref:`config_file_options`
   and set anything you think might be required on your system.
   
  .. NOTE:: 
    The above assumes that your shell will read the ``~/.bashrc`` file at
    startup time. This is not always guaranteed, depending on how your system
    is configured. If you find that the above doesn't work (e.g. typing
    ``mrview`` returns a 'command not found' error), try changing step 1 to
    instruct the ``set_path`` script to update ``PATH`` within a different
    file, for example ``~/.bash_profile`` or ``~/.profile``, e.g. as follows::

      ./set_path ~/.bash_profile

Keeping *MRtrix3* up to date
----------------------------

1. You can update your installation at any time by opening a terminal in
   the *MRtrix3* folder, and typing::

       git pull
       ./build

2. If this doesn't work immediately, it may be that you need to re-run
   the configure script::

       ./configure

   and re-run step 1 again.


Setting the CPU architecture for optimal performance
----------------------------------------------------

By default, ``configure`` will cause the build script to produce generic code
suitable for any current CPU. If you want to ensure optimal performance on your
system, you can request that ``configure`` produce code tailored to your
specific CPU architecture, which will allow it to use all available CPU
instructions and tune the code differently. This can improve performance
particularly for linear algebra operations as `Eigen
<http://eigen.tuxfamily.org>`__ will then make use of these extensions.
However, note that this means the executables produced will likely *not run* on
a different CPUs with different instruction sets, resulting in 'illegal
instruction' runtime errors. If you intend to run *MRtrix3* on a variety of
different systems with a range of CPUs, or you have no idea what the CPU is on
your target system, it is safest to avoid changing the default. 

Specifying a different CPU architecture is done by setting the ``ARCH`` environment
variable prior to invoking ``./configure``. The value of this variable will
then be passed to the compiler via the ``-march`` option. To get the best
performance *on the current system*, you can specify ``native`` as
the architecture, leaving it up to the compiler to detect your particular CPU
and its available instructions. For example::

    export ARCH=native 
    ./configure
    ./build

For more specific architectures, you can provide any value from the `list of
specifiers understood by the compiler
<https://gcc.gnu.org/onlinedocs/gcc-6.2.0/gcc/x86-Options.html#x86-Options>`_,
for example ``ARCH='sandybridge' ./configure``


.. _linux_standalone:

Standalone installation on Linux
--------------------------------

In some cases, users need to install *MRtrix3* on systems running older
distributions, over which they have little or no control, for example
centrally-managed HPC clusters. In such cases, there genuinely is no way
to install the dependencies required to compile and run *MRtrix3*. There
are two ways to address this problem: `static
executables <#static-build>`__, and the `standalone
packager <#standalone-packager>`__. With both approaches, you can
compile *MRtrix3* on a modern distro (within a virtual machine for
example), package it up, and install it on any Linux system without
worrying about dependencies.

Static build
^^^^^^^^^^^^

The simplest approach to this problem is to build so-called `static
executables <http://en.wikipedia.org/wiki/Static_library>`__, which have
no run-time dependencies. This can be accomplished by generating a
static configuration before building the software, as follows.

First, obtain the code and extract or clone it on a modern distribution
(Arch, Ubuntu 16.04, Mint 18, ..., potentially with a virtual machine if
required). Then, from the main *MRtrix3* folder::

    ./build clean
    git pull
    ./configure -static [-nogui]
    ./build

Note that this requires the availability of static versions of the
required libraries. This is generally not a problem, most distributions
will provide those by default, with the exception of Qt. If you require
a static build of MRView, you will most likely need to build a `static
version of
Qt <http://doc.qt.io/qt-5/linux-deployment.html#building-qt-statically>`__
beforehand. Use the ``-nogui`` option to skip installation of GUI
components, which rely on Qt.

You can then deploy the software onto target systems, as described in the
:ref:`deployment` section. 


Standalone packager
^^^^^^^^^^^^^^^^^^^

In the rare cases where the `static build <#Static-build>`__ procedure
above doesn't work for you, *MRtrix3* now includes the ``package_mrtrix``
script, which is designed to package an existing and fully-functional
installation from one system, so that it can be installed as a
self-contained standalone package on another system. What this means is
that you can now compile *MRtrix3* on a modern distro (within a virtual
machine for example), package it up, and install it on any Linux system
without worrying about dependencies.

**Note:** this is *not* the recommended way to install *MRtrix3*, and may
not work for your system. This is provided on a best-effort basis, as a
convenience for users who genuinely have no alternative.

What it does
""""""""""""

The ``package_mrtrix`` script is included in the top-level folder of the
*MRtrix3* package (if you don't have it, use ``git pull`` to update). In
essence, all it does is collate all the dynamic libraries necessary for
runtime operation into a single folder, which you can then copy over and
extract onto target systems. For a truly standalone installation, you
need to add the ``-standalone`` option, which will also include any
system libraries required for runtime operation from your current
system, making them available on any target system.

Limitations
"""""""""""

-  **OpenGL support:** this approach cannot magically make your system
   run ``mrview`` if it doesn't already support OpenGL 3.3 and above. This
   is a hardware driver issue, and can only be addressed by upgrading
   the drivers for your system - something that may or may not be
   possible.

-  **GUI support:** while this approach collates all the X11 libraries
   that are needed to launch the program, it is likely that these will
   then dynamically attempt to load further libraries that reside on
   your system. Unfortunately, this can introduce binary compatibility
   issues, and cause the GUI components to abort. This might happen even
   if your system does have OpenGL 3.3 support. There is unfortunately
   no simple solution to this.

-  **Installation on remote systems:** bear in mind that running the GUI
   components over a remote X11 connection is not possible, since the
   GLX protocol does not support OpenGL 3 and above (see :ref:`remote_display`
   for details). You may be able to use an OpenGL-capable VNC connection, but
   if that is not possible, there is little point in installing the GUI
   components on the remote server. The recommendation would be to configure
   with the ``-nogui`` option to skip the GUI components. You should also be
   able to access your data over the network (e.g. using SAMBA or SSHFS), in
   which case you would be able to display the images by running ``mrview``
   locally and loading the images over the shared network drives.

Instructions
""""""""""""

First, obtain the code and extract or clone it on a modern distribution
(Arch, Ubuntu 14.04, Mint 17, ..., potentially with a virtual machine if
required). Then, from the main *MRtrix3* folder::

    ./build clean
    git pull
    ./configure [-nogui]
    ./build
    ./package_mrtrix -standalone

Then copy the resulting ``_package/mrtrix3`` folder to the desired
location on the target system (maybe your own home folder). To make the
*MRtrix3* command available on the command-line, the ``bin/`` folder needs
to be added to your PATH (note this assumes that you're running the BASH
shell)::

     export PATH="$(pwd)/bin:$PATH"

Note that the above command will only add *MRtrix3* to the ``PATH`` for the
current session. You would need to add the equivalent line to your users'
startup scripts, using whichever mechanism is appropriate for your system. 


