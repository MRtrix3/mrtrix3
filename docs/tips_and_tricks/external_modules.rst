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

A module simply consists of a separate directory, which contains its own
``cmd/`` folder, and potentially also its own ``src/`` folder if required. The
build process is then almost identical to that for the MRtrix3 core, with a few
differences.

To demonstrate this, we construct a module residing
alongside the core installation::

    $ mkdir ~/mymodule
    $ cd ~/mymodule/
    $ mkdir cmd 

Assuming our module consists of a single ``mycommand.cpp`` C++ file, it should
be placed in the ``cmd/`` folder (see below for details). This results in the
following folder structure::

    mymodule/
    |-- cmd/
    |   |-- mycommand.cpp

The most relevant difference is how the build script is invoked. For a module,
compilation is started by invoking the *MRtrix3* core’s ``build`` script, but
with the module’s top-level folder being the current working directory. For
example, if the MRtrix3 core resides in the directory ``~/mrtrix3/``, and
the module resides in ``~/mymodule``, then the module can be compiled by
typing::

    $ cd ~/mymodule
    $ ../mrtrix3/build

This will compile all ``.cpp`` files found in the ``cmd/`` folder, along with
any potentially dependencies in the ``src/`` folder (if applicable), and place
the resulting executables in the ``bin/`` folder (which will be created if not
already present).

.. note::

  Once compiled, the newly created executables will only run if they remain in
  the same location relative to the *MRtrix3* core folder. This is because the
  runtime search path for the *MRtrix3* dynamic library is set within each
  executable to search in the core *MRtrix3* ``lib/`` folder (for our example,
  this is ``../../mrtrix3/lib/``). If you need to move your *MRtrix3* and
  module installations, make sure to maintain their relative paths. 



Linking to the MRtrix3 core (C++ code only)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

For routine use of modules containing C++ code, it is more convenient to set up
a reference to the core *MRtrix3* installation. This can be done in two ways:

- **Using a symbolic link** to the *MRtrix3* core’s build script::

      $ cd ~/mymodule
      $ ln -s ../mrtrix3/build

  This results in the following folder structure::

      mymodule/
      |-- bin/
      |-- build -> ../mrtrix3/build
      |-- cmd/
      |   |-- mycommand.cpp

  The link can then be invoked directly, and the build script will detect that
  it is compiling a module::

      $ ./build

  Note that this approach will NOT work on Windows / MSYS2 installations, due
  to the lack of support for symbolic links. In this case, use the alternative
  approach below.

- **Using a text file** containing the path to the *MRtrix3* core’s build script::

      $ cd ~/mymodule
      $ echo ../mrtrix3/build > build
      $ chmod +x build

  The last command ensures that the file is executable. This results in the
  following folder structure::

      mymodule/
      |-- bin/
      |-- build
      |-- cmd/
      |   |-- mycommand.cpp

  The new executable file can then be invoked directly, and the build script
  will again detect that it is compiling a module::

      $ ./build



Handling Python commands
------------------------

The instructions above relate to modules containing C++ code. Modules can also
contain Python scripts, in which case additional steps will be required to
ensure the module's scripts use the core *MRtrix3* python libraries. There are
several ways to do this, depending on your circumstances. In all cases, the aim
is to ensure that the correct ``mrtrix3.py`` module can be located and imported
(i.e. that the ``import mrtrix3`` line succeeds).

- **Symbolic link to the MRtrix3 core mrtrix3.py (recommended):** the
  core installation contains a ``bin/mrtrix3.py`` file that will be imported
  preferentially for any script co-located within the same ``bin/`` folder. It
  will in turn locate and import the actual ``mrtrix3`` module, which is
  located in the core ``lib/mrtrix3`` folder. If a symbolic link to that file
  is placed in the module's ``bin/`` folder, it will locate the correct
  modules. For example::

      $ cd ~/mymodule/
      $ ln -sr ../mrtrix3/bin/mrtrix3.py bin/

  This results in the following folder structure::

      mymodule/
      |-- bin/
      |   |-- mrtrix3.py -> ../../mrtrix3/bin/mrtrix3.py
      |-- build -> ../mrtrix3/build
      |-- cmd/
      |   |-- mycommand.cpp

- **Copy of the MRtrix3 core mrtrix3.py file:** in some cases, it may not
  be possible or convenient to use a symbolic link as described above. This is
  the case particularly on Windows / MSYS2 installations, or when distributing
  an independent module. In this case, a *copy* of the core *MRtrix3*
  ``bin/mrtrix3.py`` can be placed in the module's ``bin/`` folder::

      $ cd ~/mymodule/
      $ cp ../mrtrix3/bin/mrtrix3.py bin/

  This results in the following folder structure::

      mymodule/
      |-- bin/
      |   |-- mrtrix3.py
      |-- build -> ../mrtrix3/build
      |-- cmd/
      |   |-- mycommand.cpp

  In this case, the script will fail to detect the *MRtrix3* modules in the
  normal way, and will instead rely on the ``build`` symbolic link or file to
  locate the core libraries. For this to work, the module must therefore have
  been set up as suggested in the previous section: either with a ``build``
  symbolic link pointing the core *MRtrix3* ``build`` script, or with a
  ``build`` file containing the path to the core ``build`` script. The location
  of the core *MRtrix3* ``build`` script is then sufficient to locate the core
  Python libraries, since they should reside in a known location relative to
  that script.

- **Use the PYTHONPATH environment variable:** some users may prefer to
  set the ``PYTHONPATH`` environment variable to point to the core *MRtrix3*
  ``lib/`` folder. This is the more usual way of locating modules in Python,
  and will work here also::

      $ export PYTHONPATH=~/mrtrix3/lib

  .. note::
  
    While the ``PYTHONPATH`` environment variable will work, there are good
    reasons not to use this approach.  If you have multiple versions of
    *MRtrix3* installed on one system, and use this approach, then the Python
    modules within whichever of those *MRtrix3* versions is added to
    ``PYTHONPATH`` will *always* be imported, regardless of the version of
    *MRtrix3* against which any particular external module is *intended* to
    run.  Creation of a ``bin/mrtrix3.py`` symbolic link or copy is therefore
    preferable, as it allows different external modules to run against
    different *MRtrix3* installations.

Adding code to the module
-------------------------

New code can be added to this new module as follows:

- **Stand-alone .cpp file**: a single C++ code file destined to be compiled
  into a binary executable should have the ``.cpp`` file extension, and be
  placed into the ``cmd/`` directory of the module. Execution of the ``build``
  script in the module root directory should then detect the presence of
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

    $ cd ~/mymodule
    $ cp ~/Downloads/example_script bin/
    $ cp ~/Downloads/example_binary.cpp cmd/
    $ chmod +x bin/example_script
    $ ./build
    [1/2] [CC] tmp/cmd/example_binary.o
    [2/2] [LD] bin/example_binary

This results in the following folder structure::

    mymodule/
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
-- now reside in directory ``~/mymodule/bin/``. The ``example_binary``
executable will be linked against the core *MRtrix3* library (in the
``~/mrtrix3/lib`` folder), and the ``example_script`` Python script will
import modules from the core *MRtrix3* Python module (in the
``~/mrtrix3/lib/mrtrix3`` folder) -- neither will run if these libraries
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

         $ ~/mymodule/bin/example_binary argument1 argument2 ...

      While this may be inconvenient in some circumstances, in others it can
      be beneficial, as it is entirely explicit and clear as to exactly which
      version of the command is being run. This is especially useful when
      experimenting with different versions of a command, where the name of the
      command has not changed.

   2. Use the ``set_path`` script provided with *MRtrix3* to automatically add
      the location of the module's ``bin/`` directory to ``PATH`` whenever a
      terminal session is created. To do this, execute your core *MRtrix3*
      installation's ``set_path`` script while residing in the top-level
      directory of the module::

         $ cd ~/mymodule
         $ ../mrtrix3/set_path

   3. Manually add the location of the ``bin/`` directory of this new module to
      your system's ``PATH`` environment variable. Most likely you will want this
      location to be already stored within ``PATH`` whenever you open a new
      terminal; therefore you will most likely want to add a line such as that
      below to the appropriate configuration file for your system (e.g.
      ``~/.bashrc`` or ``~/.bash_profile``; the appropriate file will depend
      on your particular system)::

         $ export PATH=~/mymodule/bin:$PATH

      Obviously you will need to modify this line according to the location on
      your file system where you have installed the module.

