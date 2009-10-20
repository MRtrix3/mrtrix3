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

#error - "documentation.h" is for documentation purposes only!
#error - It should NOT be included in other code files.

namespace MR {

/*! \mainpage Overview
 * 
 * MRtrix was developed with simplicity and performance in mind, which has led
 * to a number of fundamental design decisions. The main concepts are explained
 * in the following pages:
 * 
 * - The build process is based on a Python script rather than Makefiles,
 * and all dependencies are resolved at build-time. This is explained in
 * \ref build_page.
 * - The basic steps for writing applications based on MRtrix are explained
 * in the section \ref command_howto.
 * - %Image data are accessed via objects that implement at least a subset of
 * the interface defined for the DataSet abstract class.
 * - Access to data stored in image files is done via the classes and
 * functions defined in the Image namespace. The corresponding headers are
 * stored in the \c lib/image/ directory.
 * 
 */

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


/*! \page command_howto How to create a new MRtrix command
 *
 * \section command_layout The anatomy of a command
 * 
 * In MRtrix, each command corresponds to a source file (\c *.cpp), placed
 * in the \c cmd/ folder, named identically to the desired application name
 * (with the .cpp suffix). The file should contain the following sections:
 * -# The relevant \#include directives, generally at least the \c lib/app.h
 * file:
 * \code
 * #include "app.h"
 * \endcode
 * Note that the \c lib/ folder is in the default search path. See \ref
 * include_path for details.
 * -# Information about the application, provided via the following macros:
 * \code 
 * SET_VERSION(0, 4, 3);
 * SET_AUTHOR("Joe Bloggs");
 * SET_COPYRIGHT("Copyright 1967 The Institute of Bogus Science");
 * \endcode
 * Alternatively, the \c SET_VERSION_DEFAULT macro will set all three of
 * these to the default values for MRtrix.
 * -# A description of what the application does, provided via the \c
 * DESCRIPTION macro:
 * \code 
 * DESCRIPTION = {
 *   "This is  brief description of the command",
 *   "A more detailed description can be provided in separate lines, each of which will generate a new paragraph.",
 *   "One more paragraph for illustration.",
 *   NULL
 * };
 * \endcode
 * This information should be provided as a NULL-terminated array of
 * strings, as illustrated above.
 * -# A list of the required command-line arguments:
 * \code
 * ARGUMENTS = {
 *   Argument ("input", "input image", "the input image.").type_image_in (),
 *   Argument ("ouput", "output image", "the output image.").type_image_out (),
 *   Argument::End
 * };
 * \endcode
 * More information about the command-line parsing interface is provided in
 * the section \ref command_line_parsing.
 * -# A list of any command-line options that the application may accept:
 * \code
 * OPTIONS = {
 *   Option ("scale", "scaling factor", "apply scaling to the intensity values.")
 *     .append (Argument ("factor", "factor", "the factor by which to multiply the intensities.").type_float (NAN, NAN, 1.0)),
 * 
 *   Option ("offset", "offset", "apply offset to the intensity values.")
 *     .append (Argument ("bias", "bias", "the value of the offset.").type_float (NAN, NAN, 0.0)),
 *
 *   Option::End
 * };
 * \endcode
 * More information about the command-line parsing interface is provided in
 * the section \ref command_line_parsing.
 * -# The main function to execute, provided via the \c EXECUTE macro:
 * \code
 * EXECUTE {
 *   ...
 *   // perform processing
 *   ...
 * }
 * \endcode
 * This function is declated \c void, and should not return any value. Any
 * errors should be handled using C++ exceptions as described below. Note
 * that any unhandled exception will cause the program to terminate with a
 * non-zero return value.
 *
 * \section error_handling Error handling
 *
 * All error handling in MRtrix is done using C++ exception. MRtrix provides
 * its own Exception class, which additionally allows an error message to be
 * displayed to the user. Developers are strongly encouraged to provide
 * helpful error messages, so that users can work out what has gone wrong
 * more easily. 
 *
 * The following is an example of an exception in use:
 *
 * \code
 * void myfunc (float param) 
 * {
 *   if (isnan (param)) throw Exception ("NaN is not a valid parameter");
 *   ...
 *   // do something useful
 *   ...
 * }
 * \endcode
 *
 * The strings helper functions can be used to provide more useful
 * information:
 *
 * \code
 * void read_file (const std::string& filename, off64_t offset) 
 * {
 *   std::ifstream in (filename.c_str());
 *   if (!in)
 *     throw Exception ("error opening file \"" + filename + "\": " + strerror (errno));
 *   
 *   in.seekg (offset);
 *   if (!in.good()) 
 *     throw Exception ("error seeking to offset " + str(offset) 
 *        + " in file \"" + filename + "\": " + strerror (errno));
 *   ...
 *   // do something useful
 *   ...
 * }
 * \endcode
 *
 * It is obviously possible to catch and handle these exceptions, as with
 * any C++ exception:
 *
 * \code
 * try {
 *   read_file ("some_file.txt", 128);
 *   // no errors, return OK:
 *   return (0);
 * }
 * catch (Exception& E) {
 *   error ("error in processing - message was: " + E.description);
 *   return (1);
 * }
 * \endcode
 *
 *
 * \section include_path Header search path
 *
 * By default, the search path provided to the compiler to look for headers
 * consists of the compiler's default path for system headers, any additional
 * paths identified by the \c configure script as necessary to meet
 * dependencies (e.g. Zlib or GSL), and the \c lib/ and \c src/ folders.
 * Headers should therefore be included by their path relative to one of these
 * folders. For example, to include the file \c lib/math/SH.h, use the
 * following:
 * \code
 * #include "math/SH.h"
 * \endcode
 *
 * Note that any non-system header must be supplied within quotes, as this
 * signals to the \c build script that this header should be taken into account
 * when generating the list of dependencies (see \ref build_section for
 * details). Conversely, any system headers must be supplied within angled
 * brackets, otherwise the \c build script will attempt (and fail) to locate it
 * within the MRtrix include folders (usually \c lib/ and \c src/) to work out
 * its dependencies, etc. For example:
 * \code 
 * // These are system headers, and will not be taken 
 * // into account when working out dependencies:
 * #include <iostream> 
 * #include <zlib.h>
 *
 * // These headers will be taken into account:
 * // any changes to these headers (or any header they themselves
 * // include) will cause the enclosing file to be recompiled
 * // the next time the \c build script encounters them.
 * #include "mrtrix.h"
 * #include "math/SH.h"
 * \endcode
 *
 * Note that this is also good practice, since it is the convention for
 * compilers to look within the system search path for header \#include'd within
 * angled brackets first, whereas headers \#include'd within quote will be
 * searched within the user search path first.
 *
 * 
 *
 */

/*! \page command_line_parsing Command-line parsing
 *
 *
 */


 /** \defgroup DataSet The DataSet abstract class
  * \brief A collection of template functions to operate on GenericDataSet objects
  *
  * MRtrix defines an abstract GenericDataSet class, describing the interface
  * that a number of MRtrix algorithms expect to operate on. It does not
  * correspond to a real class, and only serves to document the expected
  * behaviour for classes that represent image datasets.
  *
  * Classes that are designed to represent a data set should implement at
  * least a subset of the member functions documented for the GenericDataSet
  * class. Such classes should NOT derive from this class, but rather provide
  * their own implementations. There is also no requirement to reproduce the
  * function definitions exactly, as long as the class can be used with the
  * same syntax in practice. DataSet algorithms are defined using the C++
  * template framework, and hence any function call is interpreted at
  * compile-time (and potentially optimised away), rather than being issued at
  * run-time. This is perhaps better illustrated using the example below.
  *
  * The following example defines a simple class to store a 3D image:
  *
  * \code
  * class Image {
  *   public:
  *     Image (int xdim, int ydim, int zdim) { 
  *       N[0] = xdim; N[1] = ydim; N[2] = zdim;
  *       X[0] = X[1] = X[2] = 0;
  *       data = new float [N[0]*N[1]*N[2]);
  *     }
  *     ~Image () { delete [] data; }
  *
  *     typedef float value_type;
  *
  *     int     ndim () const           { return (3); }
  *     int     dim (int axis) const    { return (N[axis]); }
  *     int     pos (int axis)          { return (X[axis]); }
  *     void    pos (int axis, int i)   { X[axis] = i; }
  *     void    move (int axis, int i)  { X[axis] += i; }
  *     float   value ()                { return (data[offset()]); }
  *     void    value (float val)       { data[offset()] = val; }
  *
  *   private:
  *     float*  data
  *     int     N[3];
  *     int     X[3];
  *
  *     int     offset () const         { return (X[0]+N[0]*(X[1]+N[1]*X[2])); }
  * };
  * \endcode
  *
  * This class does not implement all the functions listed for the
  * GenericDataSet class, and some of the functions it does implement do not
  * match the equivalent GenericDataSet definitions. However, in practice this
  * class can be used with identical syntax. For example, this template
  * function scales the data by a user-defined factor:
  *
  * \code
  * template <class Set> void scale (Set& data, float factor)
  * {
  *   for (data.pos(2,0); data.pos(2) < data.dim(2); data.move(2,1))
  *     for (data.pos(1,0); data.pos(1) < data.dim(1); data.move(1,1))
  *       for (data.pos(0,0); data.pos(0) < data.dim(0); data.move(0,1))
  *         data.value (factor * data.value());
  * }
  * \endcode
  *
  * This template function might be used like this:
  *
  * \code
  * Image my_image (128, 128, 32); // create an instance of a 128 x 128 x 32 image
  * ...
  * ... // populate my_image with data
  * ...
  * scale (my_image, 10.0); // scale my_image by a factor of 10
  * \endcode
  *
  * As you can see, the \a %Image class implements all the functionality
  * required for the \a scale() function to compile and run. There is also
  * plenty of scope for the compiler to optimise this particular function,
  * since all member functions of \c my_image are declared inline. Note that
  * this does not mean that this class can be used with any of the other
  * template functions, some of which might rely on some of the other member
  * functions having been defined.
  *
  * \par Why define this abstract class?
  *
  * Different image classes may not be suited to all uses. For example, the
  * Image::Voxel class provides access to the data for an image file, but
  * incurs an overhead for each read/write access. A simpler class such as the
  * \a %Image class above can provide much more efficient access to the data.
  * There will therefore be cases where it might be beneficial to copy the
  * data from an Image::Voxel class into a more efficient data structure.
  * In order to write algorithms that can operate on all of these different
  * classes, MRtrix uses the C++ template framework, leaving it up to the
  * compiler to ensure that the classes defined are compatible with the
  * particular template function they are used with, and that the algorithm
  * implemented in the function is fully optimised for that particular class. 
  *
  * \par Why not use an abstract base class and inheritance?
  *
  * Defining an abstract class implies that all functions are declared
  * virtual. This means that every operation on a derived class will incur a
  * function call overhead, which will in many cases have a significant
  * adverse impact on performance. This also restricts the amount of optimisation that the
  * compiler might otherwise be able to perform. Using inheritance would have
  * the benefit of allowing run-time polymorphism (i.e. the same function can
  * be used with any derived class at runtime); however, in practice run-time
  * polymorphism is rarely needed in MRtrix applications. Finally, if such an
  * interface were required, it would be trivial to define such an abstract class and
  * use it with the template functions provided by MRtrix.
  *
  */


  //! \addtogroup DataSet 
  // @{

  /*! \brief The abstract generic DataSet interface
   *
   * This class is an abstract prototype describing the interface that a
   * number of MRtrix algorithms expect to operate on. For more details, see
   * \ref DataSet.
   *
   * \note The DataSet class itself should \b not be used or included in MRtrix
   * programs. Any attempt at including the relevant header will result in
   * compile-time errors.
   */
  class GenericDataSet {
    public:
      const std::string& name () const; //!< a human-readable identifier, useful for error reporting
      size_t  ndim () const; //!< the number of dimensions of the image
      int     dim (size_t axis) const; //!< the number of voxels along the specified dimension

      //! the type of data returned by the value() functions
      /*! DataSets can use a different data type to store the voxel intensities
       * than what is provided by the value() interface. For instances, it is
       * not possible to know at compile-time what type of data may be
       * contained in an input data set supplied on the command-line. The
       * Image::Voxel class (a realisation of a DataSet) provides a uniform
       * interface to the data, by translating between the datatype stored on
       * disc and the datatype requested by the application on the fly at
       * runtime. Most instances of a DataSet will probably use a \c float as
       * their \a value_type, but other types could be used in special
       * circumstances. */
      typedef float value_type;

      //! the size of the voxel along the specified dimension
      /*! The first 3 dimensions are always assumed to correspond to the \e x,
       * \e y & \e z spatial dimensions, for which the voxel size has an
       * unambiguous meaning, and should be specified in units of millimeters.
       * For the higher dimensions, the interpretation of the voxel size is
       * undefined, and may assume different meaning for different
       * applications. It may for example correspond to time in a fMRI series,
       * in which case it should be specified in seconds. Other applications
       * such as %DWI may interpret the fourth dimension as the diffusion 
       * volume direction, and leave the voxel size undefined. */
      float   vox (size_t axis) const;

      const Math::Matrix<float>& transform () const; //!< the 4x4 transformation matrix of the image.

      void    reset () //!< reset the current position to zero

      ssize_t pos (size_t axis) const; //!< return the current position along dimension \a axis
      void    pos (size_t axis, ssize_t position) const; //!< set the current position along dimension \a axis to \a position
      void    move (size_t axis, ssize_t increment) const; //!< move the current position along dimension \a axis by \a increment voxels

      value_type   value () const;          //!< return the value of the voxel at the current position
      void         value (value_type val);  //!< set the value of the voxel at the current position to \a val
  };

  // @}

}

