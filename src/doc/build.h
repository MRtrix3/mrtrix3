/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 16/08/09.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */

#error - this file is for documentation purposes only!
#error - It should NOT be included in other code files.

namespace MR {

/*! \page build_page The build process
 *
 * The procedure used to compile the source code is substantially different
 * from that used in most other open-source software. The most common
 * way to compile a software project relies on the \c make utility, and the
 * presence of one or several \c Makefiles describing which files are to
 * compiled and linked, and in what order. The process of generating the \c
 * Makefiles is often facilitated by other utilities such as \c autoconf &
 * \c automake. One disadvantage of this approach is that these \c Makefiles
 * must be updated every time changes are made to the source code that
 * affect the dependencies and the order of compilation. 
 *
 * In MRtrix, building the software relies on a two-stage process
 * implemented in Python. First,
 * the \c configure script should be executed to set the relevant
 * architecture-specific variables. Next, the \c build script is executed,
 * and is responsible for resolving all inter-dependencies, then compiling and
 * linking all the relevant files in the correct order. This means that any
 * new files added to the source tree will be compiled if needed (according
 * to the rules set out below), without any further action required. In
 * addition, this script is multithreaded and will use all available CPU
 * cores simultaneously, significantly reducing the time needed to build the
 * software on modern multi-core systems.
 *
 * \section build_process_usage Using the MRtrix build process
 *
 * The build scripts used to build MRtrix applications are designed to be
 * easy to use, with no input required from the user. This does mean that
 * developers must follow a few rules of thumb when writing software for use
 * with MRtrix.
 * - To create a new executable, place the correspondingly named source file
 * in the \c cmd/ folder. For example, if a new application called \c myapp
 * is to be written, write the corresponding code in the \c cmd/myapp.cpp
 * source file, and the build script will attempt to generate the executable
 * \c bin/myapp from it. You may want to consult the section \ref
 * command_howto.
 * - The \c lib/ folder should contain only code destined to be included into
 * the MRtrix shared library. This library is intended to provide more
 * generic image access and manipulation routines. Developers should avoid
 * placing more application-specific routines in this folder.
 * - Code designed for specific applications should be placed in the \c src/
 * folder. The corresponding code will then be linked directly into the
 * executables that make use of these routines, rather than being included
 * into the more general purpose MRtrix shared library.
 * - Non-inlined function and variable definitions should be placed in
 * appropriately named source files (\c *.cpp), and the corresponding
 * declarations should be placed in a header file with the same name and the
 * appropriate suffix (\c *.h). This is essential if the build script is to
 * resolve the correct dependencies and link the correct object files
 * together.
 * - MRtrix headers or any header added by the user must be \#include'd within
 * quotes, and any system headers within angled brackets. This is critical for
 * the build system to work out the correct dependencies (see \ref include_path
 * for details).
 * 
 * \section configure_section The configure script
 *
 * The first step required for building the software is to run the \c
 * configure script, which tailors various parameters to the specific system
 * that it is run on. This includes checking that a compiler is available and
 * behaves as expected, that other required packages are available (such as
 * GSL), whether the system is a 64-bit machine, etc. 
 * 
 * This script accepts the following options:
 * - \c -debug : generates a configuration file that will produce a version of
 * MRtrix with all debugging symbols and macros defined. This is useful when
 * developing with MRtrix to identify potential bugs early.
 * - \c -profile : generates a configuration file that will produce a version of
 * MRtrix with profiling code included. Any code compiled with this option
 * will generate a profile report that can be inspected to identify where in the
 * code the application spends most of its execution time. See 
 * http://sourceware.org/binutils/docs-2.19/gprof/index.html for details.
 * - \c -nogui : generates a configuration file that will produce only the
 * MRtrix command-line applications, leaving out any GUI applications.
 *
 * \section build_section The build script
 *
 * This script is responsible for identifying the targets to be built,
 * resolving all their dependencies, compiling all the necessary object files
 * (if they are out of date), and linking them together in the correct order.
 * This is done by first identifying the desired targets, then building a list
 * of their dependencies, and treating these dependencies themselves as targets
 * to be built first. A target can only be built once all its dependencies are
 * satisfied (i.e. all its required dependencies have been built). At this
 * point, the target is built only if one or more of its dependencies is more
 * recent than it is itself (or if it doesn't yet exist). This is done by
 * looking at the timestamps of the relevant files. In this way, the relevant
 * files are regenerated only when and if required.
 *
 * The following rules are used for each of these steps:
 *
 * \par Identifying targets to be built
 *
 * Specific targets can be specified on the command-line, in which case only
 * their minimum required dependencies will be compiled and/or linked. This
 * is useful to check that changes made to a particular file compile without
 * error, without necessarily re-compiling all other associated files. For
 * example:
 * \verbatim
$ ./build bin/mrconvert 
$ ./build lib/mrtrix.o lib/app.o \endverbatim
 * If no specific targets are given, the default target list will be
 * generated, consisting of all applications found in the \c cmd/ folder. For
 * example, if the file \c cmd/my_application.cpp exists, then the
 * corresponding target \c bin/my_application will be included in the default
 * target list.
 * 
 * \par Special targets
 *
 * There are two targets that can be passed to the \c build script that have
 * special meaning. These are:
 * - \b clean: remove all system-generated files, including all object files (\c
 *   *.o), all executables (i.e. all files in the \c bin/ folder), and the
 *   MRtrix shared library.
 * - \b reset: remove all system-generated files as above, and additionally
 *   remove the configuration file produced by the \c configure script. This
 *   should effectively reset the package to its initial state.
 *
 * \par Resolving dependencies for executables
 *
 * A target is assumed to correspond to an executable if it resides in the \c
 * bin/ folder (the default target list consists of all executables). The
 * dependencies for an example executable \c bin/myapp are resolved in the following way:
 * -# the MRtrix library \c lib/mrtrix-X_Y_Z.so is added to the list
 * -# the object file \c cmd/myapp.o is added to the list
 * -# a list of all local headers included in the source file \c
 * cmd/myapp.cpp is generated. A header is considered local if it is included
 * using inverted commas rather than angled brackets. For example:
 * \code
 * // By default, the lib/ & src/ folders are included in the include search path
 * #include "mrtrix.h"    // the file lib/mrtrix.h exists, and is considered a local header
 * #include <iostream>    // this file is not considered local
 * \endcode
 * -# if a corresponding source file is found for any of these headers, its
 * corresponding object file is added to the list. For example, if \c
 * cmd/myapp.cpp includes the header \c src/histogram.h, and the file \c
 * src/histogram.cpp exists, the object file \c src/histogram.o is added to the
 * list of dependencies. Note that object files in the \c lib/ folder are not
 * added to the list of dependencies, since they should already be included
 * in the MRtrix library (see below).
 * -# all headers included in any of the local headers or their corresponding
 * source files are also considered in the same way, recursively until no new
 * dependencies are found. For example, the file \c src/histogram.cpp might also
 * include the header \c src/min_max.h. Since the source file \c
 * src/min_max.cpp exists, the corresponding object file \c src/min_max.o
 * is added to the list.
 *
 * \par Resolving dependencies for object files
 *
 * A target is considered to be an object file if its suffix corresponds to
 * the expected suffix for an object file (usually \c *.o). The dependencies
 * for an example object file \c lib/mycode.o are resolved as follows:
 * -# the corresponding source file \c lib/mycode.cpp is added to the list
 * -# a list of all local headers included in the source file \c
 * lib/mycode.cpp is generated. 
 * -# the list of local headers is expanded by recursively adding all local
 * headers found within the already included local headers, until no new
 * local headers are found.
 * -# these headers are all added to the list of dependencies.
 *
 * \par Resolving dependencies for the MRtrix library
 *
 * The list of dependencies for the MRtrix library is generated by adding the
 * corresponding object file for each source file found in the \c lib/
 * folder. For example, if the file \c lib/image/header.cpp is found in the
 * \c lib/ folder, the object file \c lib/image/header.o is added to the list
 * of dependencies.
 *
 * \par Build rules for each target type
 *
 * - \b executables: dependencies consist of all relevant object files along
 * with the MRtrix library. These are all linked together to form the
 * executable.
 * - <b>object files:</b> dependencies consist of a single source code
 * file, along with all the included headers. The source code file is
 * compiled to form the corresponding object file.
 * - <b>MRtrix library:</b> dependencies consist of all the object files
 * found in the \c lib/ folder. These are all linked together to form the
 * shared library.
 * - <b>source & header files:</b> these have no dependencies, and require
 * no action. 
 */

}

