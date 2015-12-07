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

namespace MR
{

 /*! \page command_howto Creating a new MRtrix command

 \section command_layout The anatomy of a command

 In MRtrix, each command corresponds to a source file (\c *.cpp), placed
 in the \c cmd/ folder, named identically to the desired application name
 (with the .cpp suffix). The file should contain the following sections:

 First, the relevant headers should be \#include'd. This will include at 
 least \c lib/command.h file, which includes the relevant code for executables.
 \code
 #include "command.h"
 \endcode

 Note that the \c lib/ folder is in the default search path. See \ref
 include_path for details.
 At this point, it is helpful to bring the MR &amp; MR::App namespaces within the current
 scope:
 \code
 using namespace MR;
 using namespace App;
 \endcode

 Information about the application and its syntax is provided in the usage() function:
 \code
 void usage () {
 \endcode

 In this function, you can optionally set the author and
 copyright fields, which will be displayed when the command is invoked with
 the -version option. If ommitted, the author field will default to
 "J-Donald Tournier", and the copyright notice will default to
 that of the MRtrix core. 

 \code
 AUTHOR = "Joe Bloggs"; 

 COPYRIGHT = "whatever you want";
 \endcode

 The version information is obtained from the files \c lib/version.h (for the
 MRtrix core) and \c src/project_version.h (for external modules). Both of
 these are automatically updated to use the <a href=http://git-scm.com/>git</a>
 version information if <a href=http://git-scm.com/>git</a> is installed -
 otherwise the information already contained in these files will be used as-is.
 If you want to set the version information for your module and can't use <a
 href=http://git-scm.com/>git</a>, you can simply add the relevant information
 in the file \c src/project_version.h within your project, e.g.:

 \code
#define MRTRIX_PROJECT_VERSION "my-version-1.0"
\endcode

 You should then set a brief description of the command. Each entry
 corresponds to a new paragraph:
 \code
 DESCRIPTION 
 + "This is used to illustrate how to write a command in MRtrix"
 + "This section is where you explain a bit more about your command, "
   "its intended purpose, and any important pieces of information that "
   " might not easily fit in the following sections";
 \endcode
 
 Next, you should specify the arguments required by the command. The first
 entry for each argument is a brief name to appear on the syntax line. The
 second entry is longer description that will appear immediately after to
 explain what is expected in that argument.
 \code
 ARGUMENTS
 + Argument ("input", 
     "the input image."
     ).type_image_in()

 + Argument ("param", 
     "the parameter controlling my algorithm."
     ).type_float(0.0, 1.0, 10.0)

 + Argument ("output", 
     "the output image."
     ).type_image_out();
 \endcode
 Note that the expected type (and potentially allowed range) of each argument
 can also be specified using the type_* member functions. By default, each
 argument is a string. Also, an option can be specified to be optional, or
 that multiple such arguments are allowed. For more information, see the
 documentation for the MR::App::Argument class.

 Next, you can optionally specify any command-line options your command may
 accept. As for the MR::App::Argument class, the first entry for each option is a
 brief name, which in this case will be the name that the user will need to
 use on the command-line to invoke the option. The second entry is the
 description of the option. Optionally, each MR::App::Option can also have Arguments
 added to it. In this case, the description field of the Argument is
 ignored and should simply be ommitted. 
 \code 
 OPTIONS
 + Option ("option1", "on option requiring two arguments")
 +   Argument ("arg1").type_integer (0,1,10)
 +   Argument ("arg2")
 
 + Option ("option2", "an option that does not take arguments");
 }
 \endcode
 This marks the end of the usage() function. 

 Actual processing will take place in the run() function.  This function is
 declared \c void, and should not return any value. Any errors should be
 handled using C++ exceptions as described below. Note that any unhandled
 exception will cause the program to terminate with a non-zero return
 value. Arguments and options can be retrieved using the argument
 vector and get_options() function:
 \code
 void run ()
 {
   Image::Buffer buffer_in (argument[0]);

   Options opt = get_options ("option1");
   int arg1;
   std::string arg2;
   if (opt.size()) {
     arg1 = opt[0][0];
     arg2 = opt[0][1];
   }

   bool option2 = get_options ("option2").size();


   // perform processing...
 }
 \endcode
 Note that the retrieved arguments are implicitly type-cast to the requested
 type, without any need for explicit conversion. 

 It is also worth explaining that get_options() returns a vector of vectors
 of arguments. The first index returns the instance of the option (for cases
 where the same option can be specified multiple times), and the second
 index returns the argument supplied at that position for that instance of
 the option.   More information about the command-line parsing interface is provided in
 the section \ref command_line_parsing.



 \section error_handling Error handling

 All error handling in MRtrix is done using C++ exceptions. MRtrix provides
 its own Exception class, which additionally allows an error message to be
 displayed to the user. Developers are strongly encouraged to provide
 helpful error messages, so that users can work out what has gone wrong
 more easily.

 The following is an example of an exception in use:

 \code
 void myfunc (float param)
 {
   if (isnan (param))
     throw Exception ("NaN is not a valid parameter");
   ...
   // do something useful
   ...
 }
 \endcode

 The strings helper functions can be used to provide more useful
 information:

 \code
 void read_file (const std::string& filename, int64_t offset)
 {
   std::ifstream in (filename.c_str());
   if (!in)
     throw Exception ("error opening file \"" + filename + "\": " + strerror (errno));

   in.seekg (offset);
   if (!in.good())
     throw Exception ("error seeking to offset " + str(offset)
        + " in file \"" + filename + "\": " + strerror (errno));
   ...
   // do something useful
   ...
 }
 \endcode
 It is obviously possible to catch and handle these exceptions, as with
 any C++ exception:

 \code
 try {
   read_file ("some_file.txt", 128);
   // no errors, return OK:
   return 0;
 }
 catch (Exception& E) {
   FAIL ("error in processing - message was:");
   E.display();
   return 1;
 }
 \endcode

 \note the error message of an Exception will only be shown if it left
 uncaught, or if it is explicitly displayed using its display() member
 function.


 \section include_path Header search path

 By default, the search path provided to the compiler to look for headers
 consists of the compiler's default path for system headers, any additional
 paths identified by the \c configure script as necessary to meet
 dependencies (e.g. Zlib or Eigen), and the \c lib/ and \c src/ folders.
 Headers should therefore be included by their path relative to one of these
 folders. For example, to include the file \c lib/math/SH.h, use the
 following:
 \code
 #include "math/SH.h"
 \endcode

 Note that any MRtrix header must be supplied within quotes, as this
 signals to the \c build script that this header should be taken into account
 when generating the list of dependencies (see \ref build_section for
 details). Conversely, any system headers must be supplied within angled
 brackets, otherwise the \c build script will attempt (and fail) to locate it
 within the MRtrix include folders (usually \c lib/ and \c src/) to work out
 its dependencies, etc. For example:
 \code
 // These are system headers, and will not be taken
 // into account when working out dependencies:
 #include <iostream>
 #include <zlib.h>

 // These headers will be taken into account:
 // any changes to these headers (or any header they themselves
 // include) will cause the enclosing file to be recompiled
 // the next time the build script encounters them.
 #include "mrtrix.h"
 #include "math/SH.h"
 \endcode

 Note that this is also good practice, since it is the convention for
 compilers to look within the system search path for header \#include'd within
 angled brackets first, whereas headers \#include'd within quote will be
 searched within the user search path first.

 */

}

