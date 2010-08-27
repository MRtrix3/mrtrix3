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

/*! \page command_line_parsing Command-line parsing
 *
 * \section command_line_overview Overview
 * 
 * Command-line parsing in MRtrix is based on a set of fairly generic
 * conventions. A command is expected to accept a certain number of arguments,
 * and a certain number of options. These are specified in the code using the
 * ARGUMENTS and OPTIONS macros. They are also used to generate the help page
 * for the application, and it is therefore sensible to provide as much
 * information in the description fields as neccessary for end-users to
 * understand how to use the command.
 * 
 * Arguments are supplied as a list of Argument objects, and by default each
 * Argument is expected to have its value supplied on the command-line
 * (although one argument can be made optional, or allowed to be supplied
 * multiple times). 
 *
 * Options are supplied as a list of Option objects, and by default are
 * optional (although they can be specified as required). By default, only one
 * instance of each option is allowed, but this can also be changed. Options
 * may also accept additional arguments, which should be supplied immediately
 * after the option itself.
 *
 * Parsing of the command-line is done by first identifying any options
 * supplied and inserting them into the option list, along with their
 * corresponding arguments (if any). All remaining tokens are taken to be
 * arguments, and inserted into the App::argument list. Checks are performed
 * at this stage to ensure the number of arguments and options supplied is
 * consistent with that specified in the application.
 *
 * The values of these arguments and options can be retrieved within the
 * EXECUTE method using the App::argument list and the App::option list. Note
 * that in practice, the App::get_option() method is a much more convenient way
 * of querying the command-line options.
 *
 * \section command_line_options Option parsing
 *
 *
 * \section command_line_example Example
 *
 * Below is a fully commented example to demonstrate the use of the
 * command-line parsing interface in MRtrix.
 *
 * \code
 * #include "app.h"
 *
 * using namespace MR;
 *
 * SET_VERSION (0, 4, 3);
 * SET_AUTHOR ("Joe Bloggs");
 * SET_COPYRIGHT ("Copyright 1967 The Institute of Bogus Science");
 *
 * DESCRIPTION = {
 *   "This is  brief description of the command",
 *   "A more detailed description can be provided in separate lines, each of which will generate a new paragraph.",
 *   "One more paragraph for illustration.",
 *   NULL
 * };
 *
 * ARGUMENTS = {
 *   Argument ("input", "the input image.").type_image_in (),
 *   Argument ("ouput", "the output image.").type_image_out (),
 *   Argument ()
 * };
 *
 * OPTIONS = {
 *   Option ("scale", "apply scaling to the intensity values.")
 *     + Argument ("factor").type_float (),
 * 
 *   Option ("offset", "apply offset to the intensity values.")
 *     + Argument ("bias").type_float (),
 *
 *   Option()
 * };
 *
 * EXECUTE {
 *   ...
 *   // perform processing
 *   ...
 * }
 *  \endcode
 */

}

