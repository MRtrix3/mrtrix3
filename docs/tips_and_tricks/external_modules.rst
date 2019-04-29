.. _external_modules:

External modules
================

The *MRtrix3* build process allows for the easy development of separate modules,
compiled against the *MRtrix3* core (or indeed against any other *MRtrix3* module).
This allows developers to maintain their own repository, or compile stand-alone
commands provided by developers / other users, without affecting their core *MRtrix3*
installation. The obvious benefit is that developers can keep their own developments
private if they wish to, and the *MRtrix3* core can be kept as lean as possible.

A module simply consists of a separate directory, which contains its own ``cmd/``
folder, and potentially also its own ``src/`` folder if required. The build process
is then almost identical to that for the *MRtrix3* core, with a few differences.

The most relevant difference is how the build script is invoked. For a module,
compilation is started by invoking the *MRtrix3* core's ``build`` script, but with
the module's top-level folder being the current working directory. For example, if
the *MRtrix3* core resides in the directory ``~/src/mrtrix/core``, and the module
resides in ``~/src/mrtrix/mymodule``, then the module can be compiled by typing::

   $ cd ~/src/mrtrix/mymodule
   $ ../core/build

For routine use, it is more convenient to set up a symbolic link pointing to the
*MRtrix3* core's build script, and invoke that::

   $ cd ~/src/mrtrix/mymodule
   $ ln -s ../core/build
   $ ./build

Regardless of which technique is used to invoke the build script, there should now
be one or more compiled binaries present in the newly-created directory
``~/src/mrtrix/mymodule/bin/``.

Because these binaries are not placed into the same directory as those provided
as part of the core *MRtrix3* installation, simply typing the name of the command
into the terminal will not work, as your system will not yet be configured to
look for executable files in this new location. You can solve this in one of three
ways:

   1. Provide the *full path* to the binary file when executing it. So for
      instance, instead of typing::

         $ newcommand argument1 argument2 ...

      , you would use::

         $ ~/src/mrtrix/mymodule/bin/newcommand argument1 argument2 ...

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

         $ export PATH=/home/username/src/mrtrix/mymodule/bin:$PATH

      Obviously you will need to modify this line according to both your user
      name, and the location on your file system where you have installed the
      module.

   3. Use the ``set_path`` script provided with *MRtrix3* to automatically add
      the location of the module's ``bin/`` directory to ``PATH`` whenever a
      terminal session is created. To do this, execute the ``set_path`` script
      while residing in the top-level directory of the module::

         $ cd ~/src/mrtrix/mymodule
         $ ../core/set_path


Single-``cpp``-file commands
----------------------------

In many instances, you may be provided with a single ``.cpp`` file that contains
all of the code necessary to compile a particular command that makes use of the
*MRtrix3* libraries: a developer may choose to distribute *just* the relevant
``.cpp`` file for a particular functionality, rather than enclosing it within the
requisite directory structure required for an external *MRtrix3* module.

In such a circumstance, the steps to compile the command are as follows:

1. Create a new directory on your file system for this 'module'; for this example,
   let's suppose this is created at ``~/src/mrtrix/mymodule/``.

2. Create a sub-directory called ``cmd/`` within this directory (so the complete
   path to this new sub-directory in this instance would be: ``~/src/mrtrix/mymodule/cmd/``.

3. Place the ``.cpp`` file provided to you by the developer into the ``cmd/``
   sub-directory.

4. Within the root directory of this 'module', create a soft-link to the ``build``
   script that is stored within the root directory of your core *MRtrix3*
   installation, as described above.

5. Execute the ``build`` script from inside this module directory.

The ``build`` script should *automatically* generate a sub-directory ``bin/``
within your module directory, containing the executable file for the command
provided to you.


Python scripts
--------------

In addition to the principal binary commands, *MRtrix3* also includes a number
of Python scripts for performing common image processing tasks.  These make use
of a relatively simple set of library functions that provide a certain leven of
convenience and consistency for building such scripts (e.g. common format help
page; command-line parsing; creation, use and deletion of scratch directory;
control over command-line verbosity).

It is hoped that in addition to growing in complexity and capability over time,
this library may also be of assistance to users when building their own
processing scripts, rather than the use of e.g. Bash. The same syntax as that
used in the provided scripts can be used. If however the user wishes to run a
script that is based on this library, but is *not* located within the *MRtrix3*
``bin/`` directory, there must be some way for Python to locate the *MRtrix3*
modules in order to be able to execute the script. This can be achieved in one
of two ways:

1. Explicitly provide Python with the location of the *MRtrix3* Python modules:

    .. code-block:: console

        $ export PYTHONPATH=/home/user/mrtrix3/lib:$PYTHONPATH
        $ ./my_script [arguments] (options)

    (Replace the path to the *MRtrix3* "lib" directory with the location of your
    own installation)

2. Make a copy of the file ``bin/__locate_mrtrix.py``, and place it in the
   same directory as your external script. Upon executing your script, Python
   will use this script to attempt to locate the *MRtrix3* Python modules.


Note for Windows users
----------------------

In ``msys2``, the ``ln -s`` command actually creates a *copy* of the
target, *not* a symbolic link. By doing so, the build script is unable
to identify the location of the *MRtrix3* core libraries when trying to compile
an external module.

The simplest way around this is simply to invoke the build script of the main
*MRtrix3* install directly, as shown in the first example above.

If you *really* want a symbolic link, one solution is to use a standard Windows
command prompt, with Administrator privileges: In the file explorer, go to
``C:\Windows\system32``, locate the file ``cmd.exe``, right-click and
select 'Run as administrator'. Within this prompt, use the ``mklink``
command (note that the argument order passed to ``mklink`` is reversed
with respect to ``ln -s``; i.e. provide the location of the link, *then*
the target). Make sure that you provide the *full path* to both link and
target, e.g.::

    mklink C:\msys64\home\username\src\mrtrix\mymodule\build C:\msys64\home\username\src\mrtrix\core\build

and ``msys64`` should then be able to interpret the softlink path correctly
(confirm with ``ls -la``).

I have also found recently that the build script will not correctly detect use
of a softlink for compiling an external project when run under Python2, so
Python3 must be used explicitly.
