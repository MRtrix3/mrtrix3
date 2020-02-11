The configure script      {#configure_page}
====================

The first step in compiling MRtrix is to invoke the configure script. This
will run a series of tests to ensure the required dependencies are installed,
and figure out the appropriate compilation settings. By default, this
information is stored in the \c release/config file, ready for use with the
[build script](@ref build_page).

The configure script accepts a number of options, and is influenced by a
number of environment variables. These are all described in detail in
configure script's online help, produced by invoking it with the \c -help
ption:

     $ ./configure -help

These options should be self-explanatory, and will not be described any
further here. This section instead focuses on describing the multi-build
feature of MRtrix.

Using multiple co-existing configurations      {#multiconfig}
=========================================

By default, the configure script will generate a file called `release/config`,
with the relevant settings requested by the options supplied to the configure
script. This is a text file consisting of Python commands, which you're
encouraged to look through. It is possible to request that the information be
stored in a different folder, so that non-standard settings can be used to
build a different set of binaries, without affecting the existing set. This is
best illustrated with an example:

If we want to generate a configuration for a debug build, the `-debug` option
can be passed to the configure script to generate a suitable configuration for
it. If we also supply a suitable name as an argument, the settings will be stored
in a file called `<name>/config` instead of the default `release/config`. In
our case, we chose the name `dbg`:

    $ ./configure -debug dbg

This generates a folder called `dbg`, wihtin which the new `config` file resides. 
This allows the new `dbg` config to co-exists with the default `release` build.
We can then ask the build script to compile the debug version of the binaries
by passing the name `dbg` as the target:

    $ ./build dbg

To avoid naming conflicts with the existing binaries, all intermediate files
generated with this command will be placed in the `dbg` folder instead of the
default `release` folder. The debug version of the command can then be invoked
as, for example: 

    $ dbg/bin/mrconvert in.mif out.nii


This allows any number of configurations to co-exist without interferring with
each other. Other potentially interesting configurations include:

- one that is statically linked, configured with the `-static` option. This makes
  it easy to install on target systems where the required dependencies cannot
  be installed.

- one that does not include the GUI components, configured with the `-nogui`
  option.

- one that does not include the MRtrix shared library, configured with the
  `-noshared` option. 

- one that is built for a generic CPU, configured by setting the `ARCH`
  environment variable. This makes it easy to deploy on other systems whose
  exact CPU type might not match the system on which the executable are being
  compiled.  Note that by default, `./configure` will create a configuration to
  compile for the native CPU using the `-march=native` flag to `g++`.

For example, this creates a static configuration for a generix 64-bit CPU without 
the GUI components, and creates a target build called `server`, which can then be 
compiled alongside the default `release` build:

    $ ARCH=x86-64 ./configure -nogui -static server

    MRtrix build type requested: release [command-line only]
    
    Detecting OS: linux
    Machine architecture set by ARCH environment variable to: x86-64
    Checking for C++11 compliant compiler [g++]: 5.3.0 - tested ok
    Detecting pointer size: 64 bit
    Detecting byte order: little-endian
    Checking for variable-length array support: yes
    Checking for non-POD variable-length array support: yes
    Checking for zlib compression library: 1.2.8
    checking for Eigen 3 library: 3.2.8
    
    writing configuration to file './server/config': ok
    
    $ ./build server
    (  1/283) [CC] server/lib/image_io/sparse.o
    (  2/283) [CC] server/src/dwi/tractography/weights.o
    ...


If required, it is also possible to create a configuration by hand. This may be
useful in cases where the configuration produced by the configure script does
not work as-is, and requires manual editing. 

