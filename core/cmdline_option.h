/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __cmdline_option_h__
#define __cmdline_option_h__

#include <cassert>
#include <limits>
#include <string>

#ifdef None
# undef None
#endif

#include "mrtrix.h"
#include "types.h"

namespace MR
{
  namespace App
  {

  /*! \defgroup CmdParse Command-Line Parsing
   * \brief Classes and functions to parse command-line arguments and options.
   *
   * For a detailed description of the command-line parsing interface, see the
   * \ref command_line_parsing page.
   * */

    //! \cond skip
    enum ArgType {
      Undefined,
      Text,
      Boolean,
      Integer,
      Float,
      ArgFileIn,
      ArgFileOut,
      ArgDirectoryIn,
      ArgDirectoryOut,
      Choice,
      ImageIn,
      ImageOut,
      IntSeq,
      FloatSeq,
      TracksIn,
      TracksOut,
      Various
    };

    using ArgFlags = int;
    constexpr ArgFlags None = 0;
    constexpr ArgFlags Optional = 0x1;
    constexpr ArgFlags AllowMultiple = 0x2;
    //! \endcond




    //! \addtogroup CmdParse
    // @{

    //! A class to specify a command-line argument
    /*! Command-line arguments that are accepted by a particular command are
     * specified as a vector of Arguments objects. Please refer to \ref
     * command_line_parsing for more information.
     *
     * The list of arguments is provided by adding to the ARGUMENTS vector, like this:
     * \code
     * ARGUMENTS
     *   + Argument ("input", "the input image")
     *     .type_image_in()
     *
     *   + Argument ("parameter",
     *        "the parameter to use during processing. Allowed values are "
     *        "between 0 and 10 (default = 1).")
     *     .type_float (0.0, 10.0)
     *
     *   + Argument ("output", "the output image")
     *     .type_image_out();
     * \endcode
     * The example above specifies that the application expects exactly 3
     * arguments, with the first one being an existing image to be used as input,
     * the second one being a floating-point value, and the last one being an
     * image to be created and used as output.
     *
     * There are a number of types that the argument can be specified as. The
     * argument can also be specified as optional (see optional() function), or
     * as multiple (see allow_multiple() function). Note that in this case only
     * one such argument can be optional and/or multiple, since more than one
     * such argument would lead to ambiguities when parsing the command-line.  */
    class Argument { NOMEMALIGN
      public:
        //! constructor
        /*! this is used to construct a command-line argument object, with a name
         * and description. If default arguments are used, the object corresponds
         * to the end-of-list specifier, as detailed in \ref command_line_parsing. */
        Argument (const char* name = nullptr, std::string description = std::string()) :
          id (name), desc (description), type (Undefined), flags (None)
        {
          memset (&limits, 0x00, sizeof (limits));
        }

        //! the argument name
        const char* id;
        //! the argument description
        std::string desc;
        //! the argument type
        ArgType  type;
        //! the argument flags (AllowMultiple & Optional)
        ArgFlags flags;

        //! a structure to store the various parameters of the Argument
        union {
          const char* const* choices;
          struct { NOMEMALIGN
            int64_t min, max;
          } i;
          struct { NOMEMALIGN
            default_type min, max;
          } f;
        } limits;


        operator bool () const {
          return id;
        }

        //! specifies that the argument is optional
        /*! For example:
         * \code
         * ARGUMENTS
         *   + Argument ("input", "the input image")
         *     .type_image_in()
         *     .optional()
         *     .allow_multiple();
         * \endcode
         * \note Only one argument can be specified as optional and/or multiple.
         */
        Argument& optional () {
          flags |= Optional;
          return *this;
        }

        //! specifies that multiple such arguments can be specified
        /*! See optional() for details. */
        Argument& allow_multiple () {
          flags |= AllowMultiple;
          return *this;
        }

        //! specifies that the argument should be a text string
        Argument& type_text () {
          assert (type == Undefined);
          type = Text;
          return *this;
        }

        //! specifies that the argument should be an input image
        Argument& type_image_in () {
          assert (type == Undefined);
          type = ImageIn;
          return *this;
        }

        //! specifies that the argument should be an output image
        Argument& type_image_out () {
          assert (type == Undefined);
          type = ImageOut;
          return *this;
        }

        //! specifies that the argument should be an integer
        /*! if desired, a range of allowed values can be specified. */
        Argument& type_integer (const int64_t min = std::numeric_limits<int64_t>::min(), const int64_t max = std::numeric_limits<int64_t>::max()) {
          assert (type == Undefined);
          type = Integer;
          limits.i.min = min;
          limits.i.max = max;
          return *this;
        }

        //! specifies that the argument should be a boolean
        /*! Valid responses are 0,no,false or any non-zero integer, yes, true. */
        Argument& type_bool () {
          assert (type == Undefined);
          type = Boolean;
          return *this;
        }

        //! specifies that the argument should be a floating-point value
        /*! if desired, a range of allowed values can be specified. */
        Argument& type_float (const default_type min = -std::numeric_limits<default_type>::infinity(),
                              const default_type max = std::numeric_limits<default_type>::infinity()) {
          assert (type == Undefined);
          type = Float;
          limits.f.min = min;
          limits.f.max = max;
          return *this;
        }

        //! specifies that the argument should be selected from a predefined list
        /*! The list of allowed values must be specified as a nullptr-terminated
         * list of C strings. Here is an example usage:
         * \code
         * const char* mode_list [] = { "standard", "pedantic", "approx", nullptr };
         *
         * ARGUMENTS
         *   + Argument ("mode", "the mode of operation")
         *     .type_choice (mode_list);
         * \endcode
         * \note Each string in the list must be supplied in \b lowercase. */
        Argument& type_choice (const char* const* choices) {
          assert (type == Undefined);
          type = Choice;
          limits.choices = choices;
          return *this;
        }

        //! specifies that the argument should be an input file
        Argument& type_file_in () {
          assert (type == Undefined);
          type = ArgFileIn;
          return *this;
        }

        //! specifies that the argument should be an output file
        Argument& type_file_out () {
          assert (type == Undefined);
          type = ArgFileOut;
          return *this;
        }

        //! specifies that the argument should be an input directory
        Argument& type_directory_in () {
          assert (type == Undefined);
          type = ArgDirectoryIn;
          return *this;
        }

        //! specifies that the argument should be an output directory
        Argument& type_directory_out () {
          assert (type == Undefined);
          type = ArgDirectoryOut;
          return *this;
        }

        //! specifies that the argument should be a sequence of comma-separated integer values
        Argument& type_sequence_int () {
          assert (type == Undefined);
          type = IntSeq;
          return *this;
        }

        //! specifies that the argument should be a sequence of comma-separated floating-point values.
        Argument& type_sequence_float () {
          assert (type == Undefined);
          type = FloatSeq;
          return *this;
        }

        //! specifies that the argument should be an input tracks file
        Argument& type_tracks_in () {
          assert (type == Undefined);
          type = TracksIn;
          return *this;
        }

        //! specifies that the argument should be an output tracks file
        Argument& type_tracks_out () {
          assert (type == Undefined);
          type = TracksOut;
          return *this;
        }

        //! specifies that the argument could be one of various types
        Argument& type_various () {
          assert (type == Undefined);
          type = Various;
          return *this;
        }


        std::string syntax (int format) const;
        std::string usage () const;
    };






    //! A class to specify a command-line option
    /*! Command-line options that are accepted by a particular command are
     * specified as an array of Option objects, terminated with an empty
     * Option (constructed using default parameters). Please refer to \ref
     * command_line_parsing for more information.
     *
     * The list of options is provided using the OPTIONS macro, like this:
     * \code
     * OPTIONS
     *   + Option ("exact",
     *        "do not use approximations when processing")
     *
     *   + Option ("mask",
     *        "only perform processing within the voxels contained in "
     *        "the binary image specified")
     *     + Argument ("image").type_image_in()
     *
     *   + Option ("regularisation",
     *        "set the regularisation term")
     *     + Argument ("value").type_float (0.0, 1.0, 100.0)
     *
     *   Option ("dump",
     *        "dump all intermediate values to file")
     *     + Argument ("file").type_file();
     * \endcode
     * The example above specifies that the application accepts four options, in
     * addition to the standard ones (see \ref command_line_parsing for details).
     * The first option is a simple switch: specifying '-exact' on the
     * command line will cause the application to behave differently.
     * The next options all expect an additional argument, supplied using the
     * Argument class. Note that the description field of the Argument class is
     * unused in this case. Multiple additional arguments can be specified in
     * this way using the addition operator.
     *
     * Options can also be specified as required (see required() function), or
     * as multiple (see allow_multiple() function).
     */
    class Option : public vector<Argument> { NOMEMALIGN
      public:
        Option () : id (nullptr), flags (Optional) { }

        Option (const char* name, const std::string& description) :
          id (name), desc (description), flags (Optional) { }

        Option& operator+ (const Argument& arg) {
          push_back (arg);
          return *this;
        }
        operator bool () const {
          return id;
        }

        //! the option name
        const char* id;
        //! the option description
        std::string desc;
        //! option flags (AllowMultiple and/or Optional)
        ArgFlags flags;

        //! specifies that the option is required
        /*! An option specified as required must be supplied on the command line.
         * For example:
         * \code
         * OPTIONS
         *   + Option ("roi",
         *       "the region of interest over which to perform the processing. "
         *       "Mulitple such regions can be specified")
         *     .required()
         *     .allow_multiple()
         *     + Argument ("image").type_image_in();
         * \endcode
         */
        Option& required () {
          flags &= ~Optional;
          return (*this);
        }

        //! specifies that multiple such options can be specified
        /*! See required() for details. */
        Option& allow_multiple () {
          flags |= AllowMultiple;
          return *this;
        }

        bool is (const std::string& name) const {
          return name == id;
        }

        std::string syntax (int format) const;
        std::string usage () const;
    };






    //! a class to hold a named list of Option's
    /*! the name is used as the section heading for the options that follow.
     * For example:
     * \code
     * void usage () {
     *   ...
     *   OPTIONS
     *   + Option (...)
     *
     *   + OptionGroup ("Special options")
     *   + Option ("option1", ...)
     *   + Option ("option2", ...);
     * }
     * \endcode
     */
    class OptionGroup : public vector<Option> { NOMEMALIGN
      public:
        OptionGroup (const char* group_name = "OPTIONS") : name (group_name) { }
        const char* name;

        OptionGroup& operator+ (const Option& option) {
          push_back (option);
          return *this;
        }

        OptionGroup& operator+ (const Argument& argument) {
          assert (!empty());
          back() + argument;
          return *this;
        }

        Option& back () {
          if (empty())
            push_back (Option());
          return vector<Option>::back();
        }

        std::string header (int format) const;
        std::string contents (int format) const;
        static std::string footer (int format);
    };



  }

  //! @}
}

#endif
