.. _external_modules:

External modules
================

The *MRtrix3* build process allows for the easy development of separate modules,
compiled against the *MRtrix3* core (or indeed against any other *MRtrix3* module).
This allows developers to maintain their own repository, or compile stand-alone
commands provided by developers / other users, without affecting their core *MRtrix3*
installation. The obvious benefit is that developers can keep their own developments
private if they wish to, and the *MRtrix3* core can be kept as lean as possible.

Filesystem structure
--------------------

A module consists of a separate directory, with a similar directory structure
to the *MRtrix3* core installation, but with some symbolic links providing
reference to that *MRtrix3* core installation. To demonstrate this, first
consider the contents of an *MRtrix3* installation directory::

    $ cd ~/src/mrtrix3/

    mrtrix3/
    |-- bin/
    |   |-- (executable files)
    |-- build
    |-- build.log
    |-- build.default.active
    |   |-- (directories)
    |-- cmd/
    |   |-- (.cpp files)
    |-- config
    |-- configure
    |-- core/
    |   |-- (directories and files)
    |-- docs/
    |   |-- (directories and files)
    |-- icons/
    |   |-- (.svg files)
    |-- lib/
    |   |-- libmrtrix.so
    |   |-- mrtrix3
    |-- matlab/
    |   |-- (files)
    |-- run_pylint
    |-- run_tests
    |-- set_path
    |-- share/
    |   |-- mrtrix3
    |-- src/
    |   |-- (directories and files)
    |-- testing/
    |   |-- (directories and files)
    |-- tmp/
    |   |-- (directories)

To construct an *MRtrix3* *module*, it is necessary to both duplicate certain
aspects of this directory structure, and provide reference to the core *MRtrix3*
installation against which that module will be compiled and/or executed. Here,
we construct such a module residing alongside this core installation::

    $ cd ~/src/
    $ mkdir module
    $ cd module/
    $ mkdir cmd bin
    $ ln -s ../mrtrix3/build
    $ ln -sr ../mrtrix3/bin/mrtrix3.py bin/

    module/
    |-- bin/
    |   |-- mrtrix3.py -> ../../mrtrix3/bin/mrtrix3.py
    |-- build -> ../mrtrix3/build
    |-- cmd/
    |   |-- (empty)

This satisfies a number of requirements:

- By constructing a symbolic link to the ``build`` script of the *MRtrix3* core
  installation within the module, execution of that script will automatically
  detect that it is an external module that is being built, and will set up
  all required paths and dependencies accordingly.

  Note that setting up this symbolic link is not *strictly* required, but it
  is generally the most convenient approach. The alternative approach is to
  navigate to the root directory of the module, and then execute the ``build``
  script of the *MRtrix3* core installation from that location by providing
  its full path (relative or absolute). For example::

      $ cd ~/src/module
      $ ../mrtrix3/build

- The ``mrtrix3.py`` symbolic link in the ``bin/`` directory is used to ensure
  that any external Python scripts making use of the *MRtrix3* library modules
  are appropriately linked to the core *MRtrix3* installation against which
  they are expected to run. This makes it possible to have multiple versions of
  the *MRtrix3* library installed in different locations, and ensure that
  different scripts can locate and import the appropriate versions of the
  library at runtime.

  Note that if a module being configured consists of C++ code only, and there
  are no executable Python scripts, then providing a symbolic link to this file
  is not strictly necessary.

.. note::
   **For Windows users**:
   In ``msys2``, the ``ln -s`` command actually creates a *copy* of the
   target, *not* a symbolic link. If this is done, the build script will be unable
   to identify the location of the *MRtrix3* core libraries when trying to compile
   an external module.

   One way around this is to simply invoke the build script of the core
   *MRtrix3* installation directly, as explained above.

   If however you wish to set up the symbolic link(s), the solution is:

   1. In the Start menu, type "Command prompt" into the search bar, right-click on
      the icon corresponding to that command, and select 'Run as administrator'.

   2. Within this prompt, use the ``mklink`` command to create symbolic links.
      Note that the argument order passed to the ``mklink`` command is *reversed*
      with respect to the ``ln -s`` command; that is, you must provide the location
      where the symbolic link will be creted, and *then* the path to the target for
      the link. Additionally, make sure that you provide the *full filesystem paths*
      to both the link location and the target. So this might look something like::

         $ mklink C:\msys64\home\username\src\module\build C:\msys64\home\username\src\mrtrix\build
         $ mklink C:\msys64\home\username\src\module\bin\mrtrix3.py C:\msys64\home\username\src\mrtrix\bin\mrtrix3.py

   3. In the standard terminal used for running *MRtrix3* commands (i.e. *not* the
      Windows command prompt, but e.g. MSYS2), run the command::

         $ cd ~/src/module
         $ ls -la
         $ ls -la bin/

      Both of these filesystem paths should be reprted by the ``ls`` command as
      being symbolic links that refer back to the corresponding files in the
      *MRtrix3* core installation.

   4. Ensure that Python version 3 is used. Python version 2 has been observed
      to not correctly identify and interpret symbolic links on Windows.

Adding code to the module
-------------------------

New code can be added to this new module as follows:

- **Stand-alone .cpp file**: a single C++ code file destined to be compiled
  into a binary executable should have the ``.cpp`` file extension, and be
  placed into the ``cmd/`` directory of the module. Execution of the ``build``
  symbolic link in the module root directory should then detect the presence of
  this file, and generate an executable file in the corresponding ``bin/``
  directory.

- **Stand-alone Python file**: A stand-alone Python script designed to make use
  of the *MRtrix3* Python APIs will typically not have any file extension, and
  will have its first line set to ``#!/usr/bin/env python``. Such files should be
  placed directly into the ``bin/`` directory. It will also typically be
  necessary to mark the file as executable before the system will allow it to
  run::

   $ chmod +x bin/example_script

   (Replace ``example_script`` with the name of the script file you have added)

- **More complex modules**: If the requisite code for a particular functionality
  cannot reasonably be fully encapsulated within a single file, additional
  files will need to be added to the module. For C++ code, these will need to
  be added to the ``src/`` directory. For further details, refer to the
  relevant `developer documentation <http://www.mrtrix.org/developer-documentation/module_howto.html>`__.

For example, the following steps take the ``example_script`` Python script and
``example_binary.cpp`` C++ files, previously downloaded by the user into the
``~/Downloads/`` folder, place them in the appropriate locations in the module
created as described above, ensure the Python script is executable, and build
the C++ executable::

    $ cd ~/src/module
    $ cp ~/Downloads/example_script bin/
    $ cp ~/Downloads/example_binary.cpp cmd/
    $ chmod +x bin/example_script
    $ ./build
    [1/2] [CC] tmp/cmd/example_binary.o
    [2/2] [LD] bin/example_binary

    module/
    |-- bin/
    |   |-- example_binary
    |   |-- example_script
    |   |-- mrtrix3.py -> ../../mrtrix3/bin/mrtrix3.py
    |-- build -> ../mrtrix3/build
    |-- cmd/
    |   |-- example_binary.cpp
    |-- tmp/
    |   |-- (directories)

Both example command executables -- ``example_binary`` and ``example_script``
-- now reside in directory ``~/src/module/bin/``. The ``example_binary``
executable will be linked against the core *MRtrix3* library (in the
``~/src/mrtrix3/lib`` folder), and the ``example_script`` Python script will
import modules from the core *MRtrix3* Python module (in the
``~/src/mrtrix3/lib/mrtrix3`` folder) -- neither will run if these libraries
are not found.

Adding modules to ``PATH``
--------------------------

Because these binaries are not placed into the same directory as those provided
as part of the core *MRtrix3* installation, simply typing the name of the command
into the terminal will not work, as your system will not yet be configured to
look for executable files in this new location. You can solve this in one of three
ways:

   1. Provide the *full path* to the binary file when executing it. So for
      instance, instead of typing::

         $ example_binary argument1 argument2 ...

      you would use::

         $ ~/src/module/bin/example_binary argument1 argument2 ...

      While this may be inconvenient in some circumstances, in others it can
      be beneficial, as it is entirely explicit and clear as to exactly which
      version of the command is being run. This is especially useful when
      experimenting with different versions of a command, where the name of the
      command has not changed.

   2. Manually add the location of the ``bin/`` directory of this new module to
      your system's ``PATH`` environment variable. Most likely you will want this
      location to be already stored within ``PATH`` whenever you open a new
      terminal; therefore you will most likely want to add a line such as that
      below to the appropriate configuration file for your system (e.g.
      ``~/.bashrc`` or ``~/.bash_profile``; the appropriate file will depend
      on your particular system)::

         $ export PATH=/home/username/src/module/bin:$PATH

      Obviously you will need to modify this line according to both your user
      name, and the location on your file system where you have installed the
      module.

   3. Use the ``set_path`` script provided with *MRtrix3* to automatically add
      the location of the module's ``bin/`` directory to ``PATH`` whenever a
      terminal session is created. To do this, execute your core *MRtrix3*
      installation's ``set_path`` script while residing in the top-level
      directory of the module::

         $ cd ~/src/module
         $ ../mrtrix3/set_path

