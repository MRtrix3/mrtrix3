macOS installation
==================

We outline the steps for installing *MRtrix3* on macOS. Please consult 
the `MRtrix3 forum <http://community.mrtrix.org/>`__ if you encounter any issues
with the configure, build or runtime operations of *MRtrix3*.

Check requirements
------------------

To install *MRtrix3* , you will need the following:

-  a `C++11 <https://en.wikipedia.org/wiki/C%2B%2B11>`__ compliant
   compiler (e.g. `clang <http://clang.llvm.org/>`__ in Xcode)
-  `Python <https://www.python.org/>`__ version >= 2.7 (already included in macOS)
-  The `zlib <http://www.zlib.net/>`__ compression library (already included in macOS)
-  `Eigen <http://eigen.tuxfamily.org/>`__ version >= 3.2 
-  `Qt <http://www.qt.io/>`__ version >= 5.1 *[GUI components only]* -
   important: versions prior to this will *not* work

and optionally:

- `libTIFF <http://www.libtiff.org/>`__ version >= 4.0 (for TIFF support)
- `FFTW <http://www.fftw.org/>`__ version >= 3.0 (for improved performance in
  certain applications, currently only ``mrdegibbs``)

.. WARNING:: 

    To run the GUI components of *MRtrix3*  (``mrview`` & ``shview``), you will also need:

    - an `OpenGL <https://en.wikipedia.org/wiki/OpenGL>`__ 3.3 compliant
      graphics card and corresponding software driver - thankfully OpenGL 3.3
      is supported across the entire macOS range with OS versions >= 10.9.
    
.. NOTE:: 

    If you currently do not plan to contribute to the *MRtrix3* code, the most
    convenient way to install Mrtrix3 on macOS is to install it via homebrew. 
 
    - If you do not have homebrew installed, you can install it via::

        /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    
    - You need to add the MRtrix3 tap to homebrew: ``brew tap MRtrix3/mrtrix3``
    
    - You can now install the latest version of MRtrix3 with: ``brew install mrtrix3``
    
    This should be all you need to do. For all installation options type ``brew
    info mrtrix3``. MRtrix3 will get upgraded when you upgrade all homebrew
    packages ``brew update && brew upgrade``. If you want to avoid upgrading
    MRtrix3 the next time you upgrade homebrew you can do so with ``brew pin
    mrtrix3``.

Install Dependencies
--------------------

1. Update macOS to version 10.10 (Yosemite) or higher - OpenGL 3.3 will
   typically not work on older versions

2. Install XCode from the Apple Store

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

   - With `Homebrew <http://brew.sh/>`__:

       - Install Eigen3: ``brew install eigen``
       - Install Qt5: ``brew install qt5``
       - Install pkg-config: ``brew install pkg-config``
       - Add Qt's binaries to your path: ``export PATH=`brew --prefix`/opt/qt5/bin:$PATH``
      
   - With `MacPorts <http://macports.org/>`__:

       - Install Eigen3: ``port install eigen3``
       - Install Qt5: ``port install qt5``
       - Install pkg-config: ``port install pkgconfig``
       - Add Qt's binaries to your path: ``export PATH=/opt/local/libexec/qt5/bin:$PATH`` 
   
   - As a last resort, you can manually install Eigen3 and Qt5:
     You can use this procedure if you have good reasons to avoid the other options, or if for some reason 
     you cannot get either `Homebrew <http://brew.sh/>`__ or `MacPorts <http://macports.org/>`__ to work.

       - Install Eigen3: download and extract the source code from `eigen.tuxfamily.org <http://eigen.tuxfamily.org/>`__ 
       - Install Qt5: download and install the latest version from `<http://download.qt.io/official_releases/qt/>`__ 
           You need to select the file labelled ``qt-opensource-mac-x64-clang-5.X.X.dmg``.
           Note that you need to use at least Qt 5.1, since earlier versions
           don't support OpenGL 3.3. We advise you to use the latest version
           (5.7.0 as of the last update). You can choose to install it
           system-wide or just in your home folder, whichever suits - just
           remember where you installed it. 
       - Make sure Qt5 tools are in your PATH
           (edit as appropriate) ``export PATH=/path/to/Qt5/5.X.X/clang_64/bin:$PATH``
       - Set the CFLAG variable for eigen
           (edit as appropriate) ``export EIGEN_CFLAGS="-isystem /where/you/extracted/eigen"``
           Make sure *not* to include the final ``/Eigen`` folder in the path
           name - use the folder in which it resides instead!

4. Install TIFF and FFTW library.

   - With `Homebrew <http://brew.sh/>`__:

       - Install TIFF: ``brew install libtiff``
       - Install FFTW: ``brew install fftw``
      
   - With `MacPorts <http://macports.org/>`__:

       - Install TIFF: ``port install tiff``
       - Install FFTW: ``port install fftw-3``

Git setup
---------

Set up your git environment as per the `Git instructions
page <https://help.github.com/articles/set-up-git/#setting-up-git>`__

Build *MRtrix3* 
---------------

1. Clone the *MRtrix3*  repository::

       git clone https://github.com/MRtrix3/mrtrix3.git

   or if you have set up your SSH keys (for collaborators)::

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

1. Update the shell startup file, so that the shell can locate the *MRtrix3*
   commands and scripts, by adding the ``bin/`` folder to your ``PATH``
   environment variable.
   
   If you are not familiar or comfortable with modification of shell files,
   *MRtrix3* provides a convenience script that will perform this setup for you
   (assuming that you are using ``bash`` or equivalent interpreter).  From the
   top level *MRtrix3* directory, run the following::

       ./set_path

2. Close the terminal and start another one to ensure the startup file
   is read (or just type 'bash')

3. Type ``mrview`` to check that everything works

4. You may also want to have a look through the :ref:`config_file_options`
   and set anything you think might be required on your system.
   
  .. NOTE:: 

    The above assumes that your shell will read the ``~/.bash_profile`` file
    at startup time. This is not always guaranteed, depending on how your
    system is configured. If you find that the above doesn't work (e.g. typing
    ``mrview`` returns a 'command not found' error), try changing step 1 to
    instruct the ``set_path`` script to update ``PATH`` within a different
    file, for example ``~/.profile`` or ``~/.bashrc``, e.g. as follows::

      ./set_path ~/.profile

Keeping *MRtrix3* up to date
----------------------------

1. You can update your installation at any time by opening a terminal,
   navigating to the *MRtrix3* folder (e.g. ``cd mrtrix3``), and typing::

       git pull --tags
       ./build

2. If this doesn't work immediately, it may be that you need to re-run
   the configure script::

       ./configure

   and re-run step 1 again.


