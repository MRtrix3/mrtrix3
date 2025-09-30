The build process      {#build_page} 
=================

The procedure used to compile the source code is substantially different from
that used in most other open-source software. The most common way to compile a
software project relies on the `make` utility, and the presence of one or
several `Makefiles` describing which files are to compiled and linked, and in
what order. The process of generating the `Makefiles` is often facilitated by
other utilities such as `autoconf` & `automake`. One disadvantage of this
approach is that these `Makefiles` must be updated every time changes are made
to the source code that affect the dependencies and the order of compilation.

In MRtrix, building the software relies on a two-stage process implemented in
Python. First, the `configure` script should be executed to set the relevant
architecture-specific variables (see @ref configure_page for details). Next,
the `build` script is executed, and is responsible for resolving all
inter-dependencies, then compiling and linking all the relevant files in the
correct order. This means that any new files added to the source tree will be
compiled if needed (according to the rules set out below), without any further
action required. In addition, this script is multi-threaded and will use all
available CPU cores simultaneously, significantly reducing the time needed to
build the software on modern multi-core systems. 

@note on systems with a large number of cores but comparatively small amount 
of RAM, the multi-threaded build can run out of memory. In these cases, it may 
be necessary to reduce the number of threads used by the build script by setting 
the `NUMBER_OF_PROCESSORS` environment variable before invoking `./build`.

Using the MRtrix build process    {#build_process_usage}
==============================

The build scripts used to build MRtrix applications are designed to be
easy to use, with no input required from the user. This does mean that
developers must follow a few fixed rules when writing software for use
with MRtrix.

- To create a new executable, place the correspondingly named source file
  in the `cmd/` folder. For example, if a new application called `myapp`
  is to be written, write the corresponding code in the `cmd/myapp.cpp`
  source file, and the build script will attempt to generate the executable
  `release/bin/myapp` from it. You may want to consult the section @ref
  command_howto for information on the contents of a command.

- The `lib/` folder should contain only code destined to be included into
  the MRtrix shared library. This library is intended to provide more
  generic image access and manipulation routines. Developers should avoid
  placing more application-specific routines in this folder.

- Code designed for specific applications should be placed in the `src/`
  folder. The corresponding code will then be linked directly into the
  executables that make use of these routines, rather than being included
  into the more general purpose MRtrix shared library.

- Non-inlined function and variable definitions should be placed in
  appropriately named source files (`*.cpp`), and the corresponding
  declarations should be placed in a header file with the same name and the
  appropriate suffix (`*.h`). This is essential if the build script is to resolve
  the correct dependencies and link the correct object files together.

- MRtrix headers or any header added by the user must be included within
  quotes, and any system headers within angled brackets. This is critical for
  the build system to work out the correct dependencies (see @ref include_path
  for details).



The configure script      {#configure_section}
====================

The first step required for building the software is to run the `configure`
script, which tailors various parameters to the specific system that it is run
on. This includes checking that a compiler is available and behaves as
expected, that other required packages are available (such as Eigen), whether
the system is a 64-bit machine, etc. It is also possible to create distinct
co-existing configurations, for example to compile either release and debug
code. For details, see @ref configure_page.


The build script     {#build_section}
================

This script is responsible for identifying the targets to be built, resolving
all their dependencies, compiling all the necessary object files (if they are
out of date), and linking them together in the correct order.  This is done by
first identifying the desired targets, then building a list of their
dependencies, and treating these dependencies themselves as targets to be built
first. A target can only be built once all its dependencies are satisfied (i.e.
all its required dependencies have been built). At this point, the target is
built only if one or more of its dependencies is more recent than it is itself
(or if it doesn't yet exist). This is done by looking at the timestamps of the
relevant files. In this way, the relevant files are regenerated only when and
if required.

The following rules are used for each of these steps:

#### Identifying targets to be built

Specific targets can be specified on the command-line, in which case only
their minimum required dependencies will be compiled and/or linked. This
is useful to check that changes made to a particular file compile without
error, without necessarily re-compiling all other associated files. For
example:

    $ ./build release/bin/mrconvert
    $ ./build lib/mrtrix.o lib/app.o 


If no specific targets are given, the default target list will be generated,
consisting of all applications found in the `cmd/` folder. For example, if the
file `cmd/my_application.cpp` exists, then the corresponding target 
`release/bin/my_application` will be included in the default target list.

#### Special target: _clean_

The special target `clean` can be passed to the `build` script to remove all
system-generated files, including all object files (`*.o`), all executables
(i.e. all files in the `release/bin/` folder), and the MRtrix shared library.

#### Resolving dependencies for executables

A target is assumed to correspond to an executable if it resides in the 
`release/bin/` folder (the default target list consists of all executables).
The dependencies for an example executable \c release/bin/myapp are resolved in
the following way:

1. the MRtrix library `lib/mrtrix-X_Y_Z.so` is added to the list
 
2. the object file `cmd/myapp.o` is added to the list

3. a list of all local headers included in the source file `cmd/myapp.cpp` is
   generated. A header is considered local if it is included using inverted
   commas rather than angled brackets. For example:
   ~~~{.cpp}
   // By default, the lib/ & src/ folders are included in the include search path
   #include "mrtrix.h"    // the file lib/mrtrix.h exists, and is considered a local header
   #include <iostream>    // this file is not considered local
   ~~~

4. if a corresponding source file is found for any of these headers, its
   corresponding object file is added to the list. For example, if
   `cmd/myapp.cpp` includes the header `src/histogram.h`, and the file 
   `src/histogram.cpp` exists, the object file `src/histogram.o` is added to the
   list of dependencies. Note that object files in the `lib/` folder are not
   added to the list of dependencies, since they should already be included
   in the MRtrix library (see below).

5. all headers included in any of the local headers or their corresponding
   source files are also considered in the same way, recursively until no new
   dependencies are found. For example, the file `src/histogram.cpp` might also
   include the header `src/min_max.h`. Since the source file `src/min_max.cpp`
   exists, the corresponding object file `src/min_max.o` is added to the list.

#### Resolving dependencies for object files

A target is considered to be an object file if its suffix corresponds to the
expected suffix for an object file (usually `*.o`). The dependencies for an
example object file `lib/mycode.o` are resolved as follows:

1. the corresponding source file \c lib/mycode.cpp is added to the list

2. a list of all local headers included in the source file `lib/mycode.cpp` is
   generated.

3. the list of local headers is expanded by recursively adding all local
   headers found within the already included local headers, until no new
   local headers are found.

4. these headers are all added to the list of dependencies.

#### Resolving dependencies for the MRtrix library

The list of dependencies for the MRtrix library is generated by adding the
corresponding object file for each source file found in the `lib/` folder. For
example, if the file `lib/image/header.cpp` is found in the `lib/` folder,
the object file `lib/image/header.o` is added to the list of dependencies.

#### Build rules for each target type

- **executables:** dependencies consist of all relevant object files along with
  the MRtrix library. These are all linked together to form the
  executable.

- **object files:** dependencies consist of a single source code file, along
  with all the included headers. The source code file is compiled to form the
  corresponding object file.

- **MRtrix library**: dependencies consist of all the object files found in the
  `lib/` folder. These are all linked together to form the shared library.

- **source & header files:** these have no dependencies, and require no action.


