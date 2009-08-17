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

#ifdef DOXYGEN_SHOULD_SKIP_THIS /* Doxygen should NOT skip this! */

namespace MR {

  /*! \mainpage Overview
   * 
   * \section frontpage_section Basic principles
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
   * \c bin/myapp from it. You may want to consult the sections \ref
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
   * This is done by first identifying the desired targets, then building a
   * list of their dependencies, and treating these dependencies themselves as
   * targets, and building them first. A target can only be built once all its
   * dependencies are satisfied (i.e. all its required dependencies have been
   * built). At this point, the target is built only if one or more of its dependencies
   * is more recent than it is itself (or if it doesn't yet exist). This is
   * done by looking at the timestamps of the relevant files. In this way,
   * the relevant files are regenerated only when and if required.
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
   * dependencies are found. For example, the file \c lib/mrtrix.cpp might also
   * include the header \c lib/data_type.h. Since the source file \c
   * lib/data_type.cpp exists, the corresponding object file \c lib/data_type.o
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
    *
    * \section error_handling Error handling
    *
    * All error handling in MRtrix is done using C++ exception. MRtrix provides
    * its own Exception class, which allow an error message to be displayed to
    * the user. Developers are strongly encouraged to provide helpful error
    * messages, so that users can work out what has gone wrong more easily. 
    *
    * The following is an example of an exception in use:
    * \code
    * void myfunc (float param) 
    * {
    *   if (isnan (param)) throw Exception ("NaN is not a valid parameter");
    *   ...
    *   // do something useful
    *   ...
    * }
    * \endcode
    * The strings helper functions can be used to provide more useful
    * information:
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
    * \section command_line_parsing The command-line parsing interface
    *
    */

  /*! \page dataset_page The DataSet abstract class
    *
    * Most of the algorithms in MRtrix are designed to operate on objects that
    * implement the DataSet interface. These algorithms are implemented using
    * the C++ template framework, and are thus optimised at compile-time for
    * the particular object that the algorithm is to operate on. More details
    * on this interface are found in the DataSet definition. 
   */

  /*! \defgroup Image Image access
   * \brief Classes and functions providing access to image data. */

  //! \addtogroup Image 
  // @{


  /*! \brief The abstract generic DataSet interface
   *
   * This class is an abstract prototype describing the interface that a
   * number of MRtrix algorithms expect to operate on. It does not correspond
   * to a real class, and only serves to document the expected behaviour for
   * classes that represent image datasets.
   *
   * Classes that are designed to represent a data set should implement at
   * least a subset of the member functions described here. Such classes should
   * NOT derive from this class, but rather provide their own implementations. 
   * There is also no requirement to reproduce the function definitions
   * exactly, as long as the class can be used with the same syntax in
   * practice. Algorithms designed to operate on a DataSet are defined using the
   * C++ template framework, and hence any function call is interpreted at
   * compile-time (and potentially optimised away), rather than being issued at
   * run-time. This is perhaps better illustrated using the example below.
   *
   * The following example defines a simple class to store a 3D image:
   * \code
   * class Image {
   *   public:
   *     Image (int xdim, int ydim, int zdim) { 
   *       nvox[0] = xdim; nvox[1] = ydim; nvox[2] = zdim;
   *       pos[0] = pos[1] = pos[2] = 0;
   *       data = new float [nvox[0]*nvox[1]*nvox[2]);
   *     }
   *    ~Image () { delete [] data; }
   *
   *     int     ndim () const         { return (3); }
   *     int     dim (int axis) const  { return (nvox[axis]); }
   *     int&    operator[] (int axis) { return (pos[axis]); }
   *     float&  value()               { return (data[pos[0]+nvox[0]*(pos[1]+nvox[1]*pos[2])]); }
   *
   *   private:
   *     float*   data
   *     int      nvox[3];
   *     int      pos[3];
   * };
   * \endcode
   * This class does not implement all the functions listed for the generic
   * DataSet class, and some of the functions it does implement do not match
   * the DataSet equivalent definitions. However, in practice this
   * class can be used with identical syntax. For example, this template
   * function scales the data by a user-defined factor:
   * \code
   * template <class DataSet> void scale (DataSet& data, float factor)
   * {
   *   for (data[2] = 0; data[2] < data.dim(2); data[2]++)
   *     for (data[1] = 0; data[1] < data.dim(1); data[1]++)
   *       for (data[0] = 0; data[0] < data.dim(0); data[0]++)
   *         data.value() *= factor;
   * }
   * \endcode
   * This template function might be used like this:
   * \code
   * Image my_image (128, 128, 32); // create an instance of a 128 x 128 x 32 image
   * ...
   * ... // populate my_image with data
   * ...
   * scale (my_image, 10.0); // scale my_image by a factor of 10
   * \endcode
   * As you can see, the \a %Image class implements all the functionality
   * required for the \a scale() function to compile and run. There is also
   * plenty of scope for the compiler to optimise this particular function,
   * since all member functions of \c my_image are declared inlined. Note that this does not
   * mean that this class can be used with any of the other template functions,
   * some of which might rely on some of the other member functions having been
   * defined.
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
   * \note The DataSet class itself should \b not be used or included in MRtrix
   * programs. Any attempt at including the relevant header will result in
   * compile-time errors.
   */
    class DataSet {
      public:
        const std::string& name () const; //!< a human-readable identifier, useful for error reporting
        size_t  ndim () const; //!< the number of dimensions of the image
        int     dim (size_t axis) const; //!< the number of voxels along the specified dimension

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

        //! provides access to the ordering of the data in memory
        /*! This function should return the axis indices ordered according to
         * how contiguous in memory their data points are. This is helpful to optimise algorithms that
         * operate on image voxels independently, with no dependence on the order
         * of processing, since the algorithm can then perform the processing in
         * the order that makes best use of the memory subsystem's bandwidth.
         *
         * For example, if a 3D image is stored with all anterior-posterior
         * voxels stored contiguously in memory, and all such lines along the
         * inferior-superior axis are stored contiguously, and finally all such
         * slices along the left-right axis are stored contiguously (corresponding
         * to a stack of sagittal slices), then this function should return the
         * array [ 1, 2, 0 ]. The innermost loop of
         * an algorithm can then be made to loop over the anterior-posterior
         * direction (i.e. axis 1), which is optimal in terms of memory bandwidth.
         *
         * An algorithm might make use of this feature in the following way:
         * \code
         * template <class DataSet> void add (DataSet& data, float offset) 
         * {
         *   size_t C[data.ndim()];
         *   data.get_contiguous (C);
         *   for (data[C[2]] = 0; data[C[2]] < data.dim(C[2]); data[C[2]]++)
         *     for (data[C[1]] = 0; data[C[1]] < data.dim(C[1]); data[C[1]]++)
         *       for (data[C[0]] = 0; data[C[0]] < data.dim(C[0]); data[C[0]]++)
         *         data.value() += offset;
         * }
         * \endcode
         *
         * \note this is NOT the order as specified in the MRtrix file format,
         * but its exact inverse. */
        void get_contiguous (size_t* cont) const;


        DataType datatype () const; //!< the type of the underlying image data.
        const Math::Matrix<float>& transform () const; //!< the 4x4 transformation matrix of the image.

        const ssize_t operator[] (const size_t axis) const; //!< return the current position along dimension \a axis
        ssize_t&      operator[] (const size_t axis);       //!< manipulate the current position along dimension \a axis

        const float   value () const; //!< return the value of the voxel at the current position
        float&        value ();       //!< manipulate the value of the voxel at the current position

        bool  is_complex () const; //!< return whether the underlying data are complex

        const cfloat  Z () const; //!< return the complex value of the voxel at the current position (for complex data)
        cfloat&       Z ();       //!< manipulate the complex value of the voxel at the current position (for complex data)
    };

  //! @}
}

#endif

