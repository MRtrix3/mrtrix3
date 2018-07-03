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
be compiled binaries present in the newly-created directory
``~/src/mrtrix/mymodule/bin/``. You can then invoke such commands either by providing
the full path to the executable file, or by adding the location of the module's ``bin/``
directory to your ``PATH`` environment variable.




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