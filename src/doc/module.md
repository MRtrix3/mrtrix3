Writing separate modules     {#module_howto}
========================

The MRtrix [build process](@ref build_page) allows for the easy development of
separate modules, compiled against the MRtrix core (or indeed against any other
MRtrix module). This allows developers to maintain their own repository without
affecting the MRtrix core. The obvious benefit is that developers can keep
their own developments private if they wish to, and the MRtrix core can be kept
as lean as possible. 

A module simply consists of a separate folder, with its own `cmd/` folder,
and potentially its own `src/` folder if required. The 
[build process](@ref build_page) is then almost identical to that for the
MRtrix core, with a few differences. 

The most relevant difference is how the build script is invoked. For a module,
compilation is started by invoking the MRtrix core's build script, from the
module's top-level folder. For example, if the MRtrix core resides in the
folder `~/src/mrtrix/core`, and the module resides in `~/src/mrtrix/mymodule`,
then the module can be compiled by typing:

    $ cd ~/src/mrtrix/mymodule
    $ ../core/build

For routine use, it is more convenient to set up a symbolic link pointing to
the MRtrix core's build script, and invoke that:

    $ cd ~/src/mrtrix/mymodule
    $ ln -s ../core/build
    $ ./build


Using a symbolic link as above has the added benefit that modules can be
daisy-chained together. If a new module is created that uses code from another
module's `src` folder, the dependencies can be met simply by creating a
symbolic link (called `build`) from the new module's folder and
pointing to the required module's build script (which will itself be a
symbolic link to the MRtrix core's build script). For example, if another
module is created in the folder `~/src/mrtrix/experimental`, which builds on
code in the module in `~/src/mrtrix/mymodule` (which has already been set
up as described above), then these commands will allow it to be compiled
directly: 

    $ cd ~/src/mrtrix/experimental
    $ ln -s ../mymodule/build
    $ ./build

