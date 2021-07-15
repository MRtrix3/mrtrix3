.. _build_from_source:

Building *MRtrix3* from source
==============================

The instructions below describe the process of compiling and installing
*MRtrix3* from source. Please consult the `MRtrix3 forum
<http://community.mrtrix.org/>`__ if you encounter any issues.

.. WARNING::

  These instructions are for more advanced users who wish to install very
  specific versions of *MRtrix3*, or make their own modifications. Most
  users will find it much easier to install one of the `pre-compiled packages
  available for their platform from the main MRtrix3 website <https://www.mrtrix.org/download/>`__.


----


Install Dependencies
--------------------

To install *MRtrix3*, you will need to have a number of dependencies
available on your system, listed below. These can be installed in a
number of ways, depending on your specific platform. We provide specific
instructions for doing so for GNU/Linux, macOS and Microsoft Windows in the
subsequent sections.

Required dependencies:

-  a `C++11 <https://en.wikipedia.org/wiki/C%2B%2B11>`__ compliant
   compiler (GCC version >= 5, clang);
-  `Python <https://www.python.org/>`__ version >= 2.7 (>= 3 strongly recommended
   due to `deprecation of Python2 <https://www.python.org/doc/sunset-python-2/>`__);
-  The `zlib <http://www.zlib.net/>`__ compression library;
-  `Eigen <http://eigen.tuxfamily.org>`__ version >= 3.2 (>= 3.3 recommended);
-  `Qt <http://www.qt.io/>`__ version >= 5.5 (but *not* Qt 6) *[GUI components only]*;

and optionally:

- `libTIFF <http://www.libtiff.org/>`__ version >= 4.0 (for TIFF support);
- `FFTW <http://www.fftw.org/>`__ version >= 3.0 (for improved performance in
  certain applications, currently only ``mrdegibbs``);
- `libpng <http://www.libpng.org>`__ (for PNG support).

The instructions below list the most common ways to install these dependencies 
on Linux, macOS, and Windows platforms.

.. WARNING::

    To run the GUI components of *MRtrix3* (``mrview`` &
    ``shview``), you will also need an `OpenGL
    <https://en.wikipedia.org/wiki/OpenGL>`__ 3.3 compliant graphics card and
    corresponding software driver.

    Note that this implies you *cannot run the GUI components over a remote
    X11 connection*, since it can't support OpenGL 3.3+ rendering. The
    most up-to-date recommendations in this context can be found in the
    `relevant Wiki entry <http://community.mrtrix.org/t/remote-display-issues/2547>`__
    on the `MRtrix3 community forum <http://community.mrtrix.org>`__.

Linux
^^^^^

The installation procedure will depend on your system. Package names may
changes between distributions, and between different releases of the
same distribution. We provide commands to install the required dependencies on
some of the most common Linux distributions below.

.. WARNING::

    The commands below are **suggestions** based on what has been reported to work
    in the past, but may need to be tailored for your specific distribution.
    See below for hints on how to proceed in this case.

-  Ubuntu Linux (and derivatives, e.g. Linux Mint)::

       sudo apt-get install git g++ python libeigen3-dev zlib1g-dev libqt5opengl5-dev libqt5svg5-dev libgl1-mesa-dev libfftw3-dev libtiff5-dev libpng-dev

   .. NOTE::

         On Ubuntu 20.04 and newer, you'll to replace ``python`` in the line
         above with ``python-is-python3`` (or ``python-is-python2`` if you're
         still using version 2.7, which is now *very* deprecated).

-  RPM-based distros (Fedora, CentOS)::

       sudo yum install git g++ python eigen3-devel zlib-devel libqt5-devel libgl1-mesa-dev fftw-devel libtiff-devel libpng-devel

   On Fedora 24, this is reported to work::

       sudo yum install git gcc-c++ python eigen3-devel zlib-devel qt-devel mesa-libGL-devel fftw-devel libtiff-devel libpng-devel

-  Arch Linux::

       sudo pacman -Syu git python gcc zlib eigen qt5-svg fftw libtiff libpng

You may find that your package installer is unable to find the packages
listed, or that the subsequent steps fail due to missing dependencies
(particularly the ``./configure`` command). In this case, you will need
to search the package database and find the correct names for these
packages:

-  ``git``;

-  an appropriate C++ compiler (e.g. GCC 5 or above, or clang);

-  Python version >= 2.7 (version >= 3.0 strongly recommended);

-  the ``zlib`` compression library and its corresponding development
   header/include files;

-  the Eigen template library (only consists of development header/include files);

-  Qt version >= 5.5, its corresponding development
   header/include files, and the executables required to compile the code.
   Note that this can be broken up into several packages, depending on how your
   distribution has chosen to distribute this. You will need to get
   those that provide these Qt modules: Core, GUI, OpenGL, SVG, and the
   ``qmake``, ``rcc`` & ``moc`` executables (note these will probably be included in
   one of the other packages);

-  *[optional]* the TIFF library and utilities version >= 4.0, and its
   corresponding development header/include files;

-  *[optional]* the FFTW library version >= 3.0, and its corresponding development
   header/include files;

-  *[optional]* the PNG library and its corresponding development
   header/include files.

.. WARNING::
   Compilers included in older distributions, e.g. Ubuntu 12.04, may not be
   capable of compiling *MRtrix3*, as it requires C++11 support.
   A solution is to install a newer compiler as provided by an optional addon
   package repository, e.g. the `Ubuntu toolchain PPA
   <https://launchpad.net/~ubuntu-toolchain-r/+archive/ubuntu/test>`__. Once the
   relevant repository has been added to the distribution's package manager,
   you'll need to update the local list of available packages (e.g.
   ``sudo apt-get update``), followed by explicit installation of the newer
   version of the compiler (e.g. ``sudo apt-get install g++-7``).

.. NOTE::
   In many instances where *MRtrix3* dependencies are installed in some
   non-standard fashion, the *MRtrix3* ``configure`` script will not be able
   to automatically identify the location and/or appropriate configuration
   of those dependencies. In such cases, the *MRtrix3* ``configure`` script
   provides a range of environment variables that can be set by the user in
   order to provide this information. Executing ``configure -help`` provides
   a list of such environment variables; in addition, if the script is unable
   to detect or utilise a particular dependency properly, it will also provide a
   suggestion of which environment variable may need to be set in a manner
   tailored for your particular system in order to provide it with the
   information it needs to locate that dependency.

.. SEEALSO::
   If for whatever reasons you need to install *MRtrix3* on a system with
   older dependencies, and you are unable to update the software (e.g. you
   want to run *MRtrix3* on a centrally-managed HPC cluster), you can as a
   last resort use the `procedures described in this community forum post
   <https://community.mrtrix.org/t/standalone-installation-on-linux/3549>`__. 



macOS
^^^^^

1. Update macOS to version 10.10 (Yosemite) or higher (OpenGL 3.3 will
   typically not work on older versions);

2. Install XCode from the `App Store <https://apps.apple.com/us/app/xcode/id497799835?mt=12>`__;

3. Install Eigen3 and Qt5. 

   There are several alternative ways to do this, depending on your current
   system setup.  The most convenient is probably to use your favorite package
   manager (`Homebrew <http://brew.sh/>`__ or `MacPorts
   <http://macports.org/>`__), or install one of these if you haven't already. 
   
   If you find your first attempt doesn't work, *please* resist the temptation to
   try one of the other options: in our experience, this only leads to further
   conflicts, which won't help installing MRtrix3 *and* will make things more
   difficult to fix later. Once you pick one of these options, we strongly
   recommend you stick with it, and consult the `community forum
   <http://community.mrtrix.org>`__ if needed for advice and troubleshooting. 

   -  With `Homebrew <http://brew.sh/>`__:

       -  Install Eigen3: ``brew install eigen``
       -  Install Qt5: ``brew install qt5``
       -  Install pkg-config: ``brew install pkg-config``
       -  Add Qt's binaries to your path: ``export PATH=`brew --prefix`/opt/qt5/bin:$PATH``
      
   -  With `MacPorts <http://macports.org/>`__:

       -  Install Eigen3: ``port install eigen3``
       -  Install Qt5: ``port install qt5``
       -  Install pkg-config: ``port install pkgconfig``
       -  Add Qt's binaries to your path: ``export PATH=/opt/local/libexec/qt5/bin:$PATH``
   
   -  As a last resort, you can manually install Eigen3 and Qt5:
      You can use this procedure if you have good reasons to avoid the other options, or if for some reason
      you cannot get either `Homebrew <http://brew.sh/>`__ or `MacPorts <http://macports.org/>`__ to work.

      -  Install Eigen3: download and extract the source code from
         `eigen.tuxfamily.org <http://eigen.tuxfamily.org/>`__

      -  Install Qt5: download and install the latest version from
         `<http://download.qt.io/official_releases/qt/>`__

         You need to select the file labelled ``qt-opensource-mac-x64-clang-5.X.X.dmg``.
         You can choose to install it system-wide or just in your home folder,
         whichever suits; just remember where you installed it.

      -  Make sure Qt5 tools are in your PATH (edit as appropriate):
         ``export PATH=/path/to/Qt5/5.X.X/clang_64/bin:$PATH``

      -  Set the CFLAG variable for Eigen (edit as appropriate):
         ``export EIGEN_CFLAGS="-isystem /where/you/extracted/eigen"``
         Make sure *not* to include the final ``/Eigen`` folder in the path
         name: use the folder in which it resides instead!

4. Install TIFF, FFTW and PNG libraries.

   -  With `Homebrew <http://brew.sh/>`__:

      -  Install TIFF: ``brew install libtiff``
      -  Install FFTW: ``brew install fftw``
      -  Install PNG:  ``brew install libpng``
      
   -  With `MacPorts <http://macports.org/>`__:

      -  Install TIFF: ``port install tiff``
      -  Install FFTW: ``port install fftw-3``
      -  Install PNG:  ``port install libpng``



Windows
^^^^^^^

All of these dependencies are installed below by the MSYS2 package manager.

.. WARNING:: 

    When following the instructions below, use the **'MinGW-w64 Win64 shell'**;
    'MSYS2 shell' and 'MinGW-w64 Win32 shell' *must* be avoided, as they will
    yield erroneous behaviour that is difficult to diagnose if used accidentally.

.. WARNING::
    At time of writing, this MSYS2 system update will give a number of
    instructions, including: terminating the terminal when the update is
    completed, and modifying the shortcuts for executing the shell(s). Although
    these instructions are not as prominent as they could be, it is *vital*
    that they are followed correctly!


1. Download and install the most recent 64-bit MSYS2 installer from
   http://msys2.github.io/ (msys2-x86\_64-\*.exe), and following the
   installation instructions from the `MSYS2 wiki <https://github.com/msys2/msys2/wiki/MSYS2-installation>`__. 

2. Run the program **'MinGW-w64 Win64 Shell'** from the start menu.

3. Update the system packages, `as per the instructions
   <https://github.com/msys2/msys2/wiki/MSYS2-installation#iii-updating-packages>`__::

       pacman -Syuu

   Close the terminal, start a new **'MinGW-w64 Win64 Shell'**, and repeat as
   necessary until no further packages are updated. 

4. From the **'MinGW-w64 Win64 Shell'** run::

       pacman -S git python pkg-config mingw-w64-x86_64-gcc mingw-w64-x86_64-eigen3 mingw-w64-x86_64-qt5 mingw-w64-x86_64-fftw mingw-w64-x86_64-libtiff mingw-w64-x86_64-libpng
    
   Sometimes ``pacman`` may fail to find a particular package from any of
   the available mirrors. If this occurs, you can download the relevant
   package from `SourceForge <https://sourceforge.net/projects/msys2/files/REPOS/MINGW/x86_64/>`__:
   place both the package file and corresponding .sig file into the
   ``/var/cache/pacman/pkg`` directory, and repeat the ``pacman`` call above.

   Sometimes ``pacman`` may refuse to install a particular package, claiming e.g.::

       error: failed to commit transaction (conflicting files)
       mingw-w64-x86_64-eigen3: /mingw64 exists in filesystem
       Errors occurred, no packages were upgraded.

   Firstly, if the offending existing target is something trivial that can
   be deleted, this is all that should be required. Otherwise, it is possible
   that MSYS2 may mistake a *file* existing on the filesystem as a
   pre-existing *directory*; a good example is that quoted above, where
   ``pacman`` claims that directory ``/mingw64`` exists, but it is in fact the
   two files ``/mingw64.exe`` and ``/mingw64.ini`` that cause the issue.
   Temporarily renaming these two files, then changing their names back after
   ``pacman`` has completed the installation, should solve the problem.


----


Git setup
---------

If you intend to contribute to the development of *MRtrix3*, set up your git
environment as per the `Git instructions page
<https://help.github.com/articles/set-up-git/#setting-up-git>`__


----


.. _build_mrtrix3:

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


----


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


----


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



