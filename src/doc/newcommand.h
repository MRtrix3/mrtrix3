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
 * -# At this point, it is helpful to bring the MR namespace within the current
 * scope:
 * \code
 * using namespace MR;
 * \endcode
 * -# Information about the application, provided via the following macros:
 * \code 
 * SET_VERSION (0, 4, 3);
 * SET_AUTHOR ("Joe Bloggs");
 * SET_COPYRIGHT ("Copyright 1967 The Institute of Bogus Science");
 * \endcode
 * The \c SET_VERSION_DEFAULT macro can also be used to set the version to the
 * same as the main MRtrix library. If either the author or the copyright are
 * set to NULL, default values will be used instead.
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
 * All error handling in MRtrix is done using C++ exceptions. MRtrix provides
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
 *   if (isnan (param))
 *     throw Exception ("NaN is not a valid parameter");
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
 * void read_file (const std::string& filename, int64_t offset) 
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
 *   error ("error in processing - message was:");
 *   E.display();
 *   return (1);
 * }
 * \endcode
 *
 * \note the error message of an Exception will only be shown if it left
 * uncaught, or if it is explicitly displayed using its display() member
 * function. 
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
 * // the next time the build script encounters them.
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

}

