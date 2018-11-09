Windows installation
====================


We outline the steps for installing *MRtrix3* for Windows using 
`MSYS2 <https://github.com/msys2/msys2/wiki>`__. 
Please consult the `MRtrix3 forum <http://community.mrtrix.org/>`__ if you
encounter any issues with the configure, build or runtime operations of
*MRtrix3*.

.. WARNING::
    Some of the Python scripts provided with *MRtrix3* are dependent on
    external software tools (for instance FSL). If these packages are
    not available on Windows, then the corresponding *MRtrix3* scripts
    also cannot be run on Windows. A virtual machine may therefore be
    required in order to use these particular scripts; though *MRtrix3*
    may still be installed natively on Windows for other tasks.

Check requirements
------------------

To install *MRtrix3*, you will need the following:

-  a `C++11 <https://en.wikipedia.org/wiki/C%2B%2B11>`__ compliant
   compiler
-  `Python <https://www.python.org/>`__ version >= 2.7
-  The `zlib <http://www.zlib.net/>`__ compression library
-  `Eigen <http://eigen.tuxfamily.org>`__ version >= 3.2
-  `Qt <http://www.qt.io/>`__ version >= 4.7 *[GUI components only]*

and optionally:

- `libTIFF <http://www.libtiff.org/>`__ version >= 4.0 (for TIFF support)
- `FFTW <http://www.fftw.org/>`__ version >= 3.0 (for improved performance in
  certain applications, currently only ``mrdegibbs``)

.. NOTE::
    All of these dependencies are installed below by the MSYS2 package manager.

.. WARNING:: 
    To run the GUI components of *MRtrix3* (``mrview`` & ``shview``), you will also need:

    - an `OpenGL <https://en.wikipedia.org/wiki/OpenGL>`__ 3.3 compliant
      graphics card and corresponding software driver 

.. WARNING:: 

    When following the instructions below, use the **'MinGW-w64 Win64 shell'**;
    'MSYS2 shell' and 'MinGW-w64 Win32 shell' should be avoided.

Install and update MSYS2
------------------------

1. Download and install the most recent 64-bit MSYS2 installer from
   http://msys2.github.io/ (msys2-x86\_64-\*.exe), and following the
   installation instructions from the `MSYS2 wiki <https://github.com/msys2/msys2/wiki/MSYS2-installation>`__. 

2. Run the program **'MinGW-w64 Win64 Shell'** from the start menu.

3. Update the system packages, `as per the instructions
   <https://github.com/msys2/msys2/wiki/MSYS2-installation#iii-updating-packages>`__::

       pacman -Syuu

   Close the terminal, start a new **'MinGW-w64 Win64 Shell'**, and repeat as
   necessary until no further packages are updated. 

.. WARNING::
    At time of writing, this MSYS2 system update will give a number of
    instructions, including: terminating the terminal when the update is
    completed, and modifying the shortcuts for executing the shell(s). Although
    these instructions are not as prominent as they could be, it is *vital*
    that they are followed correctly!

Install *MRtrix3* dependencies
------------------------------

1. From the **'MinGW-w64 Win64 Shell'** run::

        pacman -S git python pkg-config mingw-w64-x86_64-gcc mingw-w64-x86_64-eigen3 mingw-w64-x86_64-qt5 mingw-w64-x86_64-fftw mingw-w64-x86_64-libtiff
    
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
   that MSYS2 may mistake a _file_ existing on the filesystem as a
   pre-existing _directory_; a good example is that quoted above, where
   ``pacman`` claims that directory ``/mingw64`` exists, but it is in fact the
   two files ``/mingw64.exe`` and ``/mingw64.ini`` that cause the issue.
   Temporarily renaming these two files, then changing their names back after
   ``pacman`` has completed the installation, should solve the problem.


Git setup
---------

If you intend to contribute to the development of *MRtrix3*, set up your git
environment as per the `Git instructions page
<https://help.github.com/articles/set-up-git/#setting-up-git>`__



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
   


Keeping *MRtrix3* up to date
----------------------------

1. You can update your installation at any time by typing::

       git pull
       ./build

2. If this doesn't work immediately, it may be that you need to re-run
   the configure script::

       ./configure

   and re-run step 1 again.

Compiling external projects with ``msys2``
------------------------------------------

In ``msys2``, the ``ln -s`` command actually creates a *copy* of the
target, *not* a symbolic link. By doing so, the build script is unable
to identify the location of the MRtrix libraries when trying to compile
an external module.

The simplest way around this is simply to invoke the build script of the main
*MRtrix3* install directly. For example, if compiling an external project called
``myproject``, residing in a folder alongside the main ``mrtrix3`` folder, the
build script can be invoked with::

    # current working directory is 'myproject':
    ../mrtrix3/build

If you really want a symbolic link, one solution is to use a standard Windows
command prompt, with Administrator privileges: In the file explorer, go to
``C:\Windows\system32``, locate the file ``cmd.exe``, right-click and
select 'Run as administrator'. Within this prompt, use the ``mklink``
command (note that the argument order passed to ``mklink`` is reversed
with respect to ``ln -s``; i.e. provide the location of the link, *then*
the target). Make sure that you provide the *full path* to both link and
target, e.g.::

    mklink C:\msys64\home\username\src\my_project\build C:\msys64\home\username\src\MRtrix3\build

and ``msys64`` should be able to interpret the softlink path correctly
(confirm with ``ls -la``).

I have also found recently that the build script will not correctly detect use
of a softlink for compiling an external project when run under Python2, so
Python3 must be used explicitly.
 
