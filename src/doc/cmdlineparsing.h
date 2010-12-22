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
   * optional (although they can be specified as 'required'). By default, only
   * one instance of each option is allowed, but this can also be changed.
   * Options may also accept additional arguments, which should be supplied
   * immediately after the option itself.
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
   * that in practice, the App::get_options() method is a much more convenient way
   * of querying the command-line options.
   *
   * \section command_line_header Getting started
   *
   * To create a new command, a new file should be added to the cmd/ folder, with
   * the same name as the intended command and the '.cpp' extension. The
   * minimum contents of the file should be:
   * \code
   * #include "app.h"
   *
   * using namespace MR;
   *
   * SET_VERSION (0, 4, 3);
   * SET_AUTHOR ("Joe Bloggs (j.bloggs@bogus.org)");
   * SET_COPYRIGHT ("Copyright 1967 The Institute of Bogus Science");
   *
   * DESCRIPTION = {
   *   "A brief description of the command",
   *   NULL
   * };
   *
   * ARGUMENTS = { Argument () };
   *
   * OPTIONS = { Option() };
   *
   * EXECUTE { }
   * \endcode
   *
   * The final EXECUTE macro corresponds the main function, and is where
   * processing actually takes place. Retrieval of the argument and option values
   * is performed in the body of this function.
   *
   * The SET_VERSION, SET_AUTHOR and SET_COPYRIGHT macros are required, and
   * should provide the corresponding information. As an alternative, it is also
   * possible to use the default MRtrix values:
   * \code
   * SET_VERSION_DEFAULT;
   * SET_AUTHOR (NULL);
   * SET_COPYRIGHT (NULL);
   * \endcode
   *
   * \subsection command_line_description Providing a description of the command
   *
   * The DESCRIPTION macro is used to specify a description of the command, which
   * will be listed on its help page. It consists of a NULL-terminated array of
   * character strings. Each string will correspond to a paragraph in the help
   * page.
   *
   * \subsection command_line_arguments Specifying command-line arguments
   *
   * The ARGUMENTS macro is used to provide a list of the command-line arguments.
   * This is provided as an array of Argument objects, terminated with a
   * default-constructed Argument(). As a minimum, each Argument is constructed
   * with its short-hand name (used in the syntax line), and its description:
   * \code
   * ARGUMENTS = {
   *   Argument ("input", "a description of the input argument"),
   *   Argument ()
   * }
   * \endcode
   *
   * By default, an Argument is typed as a string. There are a number of other
   * types that can be used, and each type will perform its own checks on the
   * values provided by the user. For example:
   * \code
   * ARGUMENTS = {
   *   Argument ("input", "a description of the input argument").type_integer (0, 8, 16),
   *   Argument ()
   * }
   * \endcode
   *
   * The type specifiers that can be used are:
   * <dl>
   * <dt>type_text ()</dt><dd>the default</dd>
   * <dt>type_image_in ()</dt><dd>argument corresponds to an already-existing
   * image that will be used as an input. The argument value will be retrieved as
   * a string corresponding to the image specifier.</dd>
   * <dt>type_image_out ()</dt><dd>argument corresponds to a non-existing image
   * that will be produced by the program. The argument value will be retrieved
   * as a string corresponding to the image specifier.</dd>
   * <dt>type_integer (minimum, default, maximum)</dt><dd>argument corresponds to
   * an integer value in the range [minimum, maximum], with default value as
   * specified. The argument value will be retrieved as an int. If (minimum,
   * default, maximum) are ommitted, default values will be used.</dd>
   * <dt>type_float (minimum, default, maximum)</dt><dd>argument corresponds to
   * a floating-point value in the range [minimum, maximum], with default value as
   * specified. The argument value will be retrieved as a float or double. If (minimum,
    default, maximum) are ommitted, default values will be used.</dd>
   * <dt>type_choice (list)</dt><dd>argument corresponds to one of a set number
   * of available choices, supplied as a NULL-terminated array of strings. The
   * argument value can be retrieved as a string or as an int, corresponding to
   * the appropriate index into the list of choices. </dd>
   * <dt>type_sequence_int ()</dt><dd>argument corresponds to a sequence of
   * integers. The argument value will be retrieved as a std::vector<int>.</dd>
   * <dt>type_sequence_float ()</dt><dd>argument corresponds to a sequence of
   * floating-point values. The argument value will be retrieved as a
   * std::vector<float>.</dd>
   * </dl>
   *
   * In certain cases, it is possible that none of these types are suitable. In
   * these cases, the default type (type_text) should be used, and any custom
   * casting and checking will need to be performed explicitly within the EXECUTE
   * function.
   *
   * It is possible to specify at most one argument as being optional, by adding
   * the 'optional' flag:
   * \code
   * ARGUMENTS = {
   *   Argument ("input", "a description of the input argument").optional(),
   *   Argument ()
   * }
   * \endcode
   *
   * Finally, at most one argument can be specified as repeatable (in other
   * words, a number of these arguments can be supplied one after the other).
   * This is useful for example if the command can operate on multiple data sets
   * sequentially:
   * \code
   * ARGUMENTS = {
   *   Argument ("input", "a description of the input argument").allow_multiple(),
   *   Argument ()
   * }
   * \endcode
   *
   *
   * \subsection command_line_options Specifying command-line options
   *
   * The OPTIONS macro is used to provide a list of the options understood by the
   * program. This is supplied as a list of Option objects, terminated with a
   * default-constructed Option(). As a minimum, each Option is constructed
   * with its short-hand name (used on the command-line), and its description:
   * \code
   * OPTIONS = {
   *   Option ("option", "a description of the option"),
   *   Option ()
   * }
   * \endcode
   *
   * Similarly to arguments, options can be specified as repeatable:
   * \code
   * OPTIONS = {
   *   Option ("option", "a description of the option").allow_multiple(),
   *   Option ()
   * }
   * \endcode
   *
   * Options can also be specified as required (by default, options are
   * optional):
   * \code
   * OPTIONS = {
   *   Option ("option", "a description of the option").required(),
   *   Option ()
   * }
   * \endcode
   *
   * To handle additional arguments to an Option, Arguments can be added to the
   * option:
   * \code
   * OPTIONS = {
   *   Option ("option", "a description of the option")
   *     + Argument ("value").type_integer ()
   *     + Argument ("list").type_sequence_int (),
   *   Option ()
   * }
   * \endcode
   *
   * Note that in this case there is no need to specify a description for these
   * additional arguments, since any relevant documentation for that argument is
   * expected to be provided in the description of its parent Option. These
   * Arguments can be typed in the same way as regular Arguments.
   *
   * \section command_line_retrieve Retrieving command-line argument and option values
   *
   * Argument and option values can be retrieved within the body of the EXECUTE
   * function. Arguments are provided via the \c argument vector, which is a
   * std::vector<App::ParsedArgument>. As such, values can easily be obtained
   * using its subscript operator. For types other than strings, values can be
   * cast to the correct type by implicit type-casting (i.e. simply by using the
   * argument in a context that requires the relevant type - e.g.  assignment).
   * The following examples should clarify how this is done:
   * \code
   * EXECUTE {
   *   size_t numarg = argument.size(); // the number of arguments
   *
   *   // retrieving values as a string:
   *   Image::Header header (argument[0]);
   *
   *   // retrieving value by implicit type-casting:
   *   // Note that this argument is expected to have been specified using type_integer().
   *   // Bound checks are performed at this stage.
   *   int value = argument[1];
   *
   *   // Another example of retrieval via implicit type-casting:
   *   // Note that in this case, the argument is expected to have been specified using type_sequence_int()
   *   std::vector<int> list = argument[2];
   *
   *   ...
   * }
   * \endcode
   *
   * Option values are most easily retrieved using the get_options() convenience
   * function. This returns a Options object that is to all intents and purposes
   * identical in use to a std::vector< std::vector<App::ParsedArgument> >: one
   * vector of values for each matching option specified on the command-line (an
   * option may be specified multiple times if it has been specified using
   * allow_multiple).  The values are stored as App::ParsedArgument objects (as
   * for the parsed arguments), and their values can therefore also be retrieved
   * using implicit type-casting (see above). The example below should clarify
   * this:
   * \code
   * EXECUTE {
   *   Options opt = get_options ("option");
   *
   *   if (opt.size()) { // check whether any such options have been supplied
   *     // retrieve first argument of first option as an int:
   *     int value = opt[0][0];
   *
   *     // retrieve second argument of first option as a sequence of ints:
   *     std::vector<int> list = opt[0][1];
   *   }
   *
   *   // in cases where the option has been specified using allow_multiple(),
   *   // opt.size() will return the number of such options.
   *   // In this example, the first argument of each additional option is added
   *   // to the list of values by implicit type-casting to a float:
   *   opt = get_options ("multiple_option");
   *   std::vector<float> values;
   *   for (size_t n = 0; n < opt.size(); ++n)
   *     values.push_back (opt[n][0]);
   *
   *   ...
   * }
   * \endcode
   *
   * If for any reason the get_options() convenience function is not suitable,
   * options can be accessed directly using the \c option variable, which is a
   * std::vector of App::ParsedOption objects, one per option in the same order
   * as on the command-line. Note that in this case, option values will only be
   * available as const char* string arrays, and no bounds checking will be
   * performed. For further details, refer to the App::ParsedOption description
   * page.

   */

}

