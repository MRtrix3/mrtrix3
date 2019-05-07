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

A module consists of a separate directory, which possesses a similar directory
structure to the *MRtrix3* core installation, but with some symbolic links providing
reference to that *MRtrix3* core installation. To demonstrate this, first consider
the contents of an *MRtrix3* installation directory::

    $ cd ~/src/mrtrix

    core/
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
installation against which that module will be executed / run. Here, we construct
such a module residing alongside this core installation::

    $ cd ~/src/mrtrix
    $ mkdir module
    $ mkdir module/bin
    $ mkdir module/cmd
    $ ln -s core/build module/
    $ ln -s core/bin/mrtrix3.py module/bin/

    module/
    |-- bin/
    |   |-- mrtrix3.py -> ../../core/bin/mrtrix3.py
    |-- build -> ../core/build
    |-- cmd/
    |   |-- (empty)

This satisfies a number of requirements:

-  By constructing a symbolic link to the ``build`` script of the *MRtrix3* core
   installation within the module, execution of that script will automatically
   detect that it is an external module that is being built, and will set up
   all required paths and dependencies accordingly.

   Note that while this step is not *strictly* required - it is in fact
   satisfactory to navigate to the root directory of the module, and then execute
   the ``build`` script of the *MRtrix3* core installation from that location,
   and the script will still detect and compile the external module - setting up
   the symbolic link is generally the most convenient approach.

   So this would look like::

       $ cd ~/src/mrtrix/module
       $ ../core/build

-  File "``mrtrix3.py``" residing in the ``bin/`` directory is utilised to
   ensure that any external Python scripts making use of the *MRtrix3* library
   modules are appropriately linked to the core *MRtrix3* installation against
   which they are written.

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

         $ mklink C:\msys64\home\username\src\mrtrix\module\build C:\msys64\home\username\src\mrtrix\core\build
         $ mklink C:\msys64\home\username\src\mrtrix\module\bin\mrtrix3.py C:\msys64\home\username\src\mrtrix\core\bin\mrtrix3.py

   3. In the standard terminal used for running *MRtrix3* commands (i.e. *not* the
      Windows command prompt, but e.g. MSYS2), run the command::

         $ cd ~/src/mrtrix/module
         $ ls -la
         $ ls -la bin/

      Both of these filesystem paths should be reprted by the ``ls`` command as
      being symbolic links that refer back to the corresponding files in the
      *MRtrix3* core installation.

   4. Ensure that Python version 3 is used. Python version 2 has been observed
      to not correctly identify and interpret symbolic links on Windows.

Incorporating new code
----------------------

Code corresponding to the functionalities of this new module can then be utilised
as follows:

-  **Stand-alone ``.cpp`` file**: Often, when a stand-alone functionality built
   against the *MRtrix3* C++ APIs, it will be distributed as a single code file
   with the "``.cpp``" file extension. In these circumstances, such a file should
   be placed into the ``cmd/`` directory of the module. Execution of the ``build``
   symbolic link in the module root directory should detect the presence of this
   file, and generate an executable file in the corresponding ``bin/`` directory.

-  **Stand-alone Python file**: A stand-alone Python script designed to make use
   of the *MRtrix3* Python APIs will typically not have any file extension. Such
   files should be placed directly into the ``bin/`` directory.

   Generally, having downloading such a file, the user's system will not permit
   execution of such, due to the potential of being malicious code. To enable
   direct execution of such a script, it will likely be necessary to manually
   identify the file as being executable::

   $ chmod +x bin/example_script

   (Replacing "``example_script``" with the name of the script file you have
   downloaded)

-  **More complex modules**: If the requisite code for a particular functionality
   cannot reasonably be fully encapsulated within a single file, then the creator
   of that module would typically provide the code for that functionality *already
   arranged* in such a way to be identifiable as an *MRtrix3* module. In such a
   circumstance, the user would only be required to set up the symbolic links to
   the ``build`` and ``bin/mrtrix3.py`` files.

Following these steps, the setup and compilation of an external module may look
something like this::

    $ cd ~/src/mrtrix
    $ cp ~/Downloads/example_script module/bin/
    $ cp ~/Downloads/example_binary.cpp module/cmd/
    $ chmod +x module/bin/example_script
    $ ./module/build
    [1/2] [CC] tmp/cmd/example_binary.o
    [2/2] [LD] bin/example_binary

    module/
    |-- bin/
    |   |-- example_binary
    |   |-- example_script
    |   |-- mrtrix3.py -> ../../core/bin/mrtrix3.py
    |-- build -> ../core/build
    |-- cmd/
    |   |-- example_binary.cpp
    |-- tmp/
    |   |-- (directories)

Both example command executables - ``example_binary`` and ``example_script`` -
now reside in directory ``~/src/mrtrix/module/bin/``.

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

      , you would use::

         $ ~/src/mrtrix/module/bin/example_binary argument1 argument2 ...

      While this may be inconvenient in some circumstances, in others it can
      be beneficial, as it is entirely explicit and clear as to where the command
      is being run from. This is especially the case when experimenting with
      different versions of a command where the name of the command has not changed.

   2. Manually add the location of the ``bin/`` directory of this new module to
      your system's ``PATH`` environment variable. Most likely you will want this
      location to be already stored within ``PATH`` whenever you open a new
      terminal; therefore you will most likely want to add a line such as that
      below to the appropriate configuration file for your system (e.g.
      ``~/.bashrc`` or ``~/.bash_profile``; the appropriate file will depend
      on your particular system)::

         $ export PATH=/home/username/src/mrtrix/module/bin:$PATH

      Obviously you will need to modify this line according to both your user
      name, and the location on your file system where you have installed the
      module.

   3. Use the ``set_path`` script provided with *MRtrix3* to automatically add
      the location of the module's ``bin/`` directory to ``PATH`` whenever a
      terminal session is created. To do this, execute the ``set_path`` script
      while residing in the top-level directory of the module::

         $ cd ~/src/mrtrix/module
         $ ../core/set_path
