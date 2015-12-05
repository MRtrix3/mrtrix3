/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __cmdline_option_h__
#define __cmdline_option_h__

#include <cassert>
#include <string>
#include <vector>
#include <limits>

#ifdef None
# undef None
#endif

#include "mrtrix.h"

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
    typedef enum {
      Text,
      Boolean,
      Integer,
      Float,
      ArgFileIn,
      ArgFileOut,
      Choice,
      ImageIn,
      ImageOut,
      IntSeq,
      FloatSeq
    } ArgType;

    typedef int ArgFlags;
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
     *     .type_float (0.0, 1.0, 10.0)
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
    class Argument
    {
      public:
        //! constructor
        /*! this is used to construct a command-line argument object, with a name
         * and description. If default arguments are used, the object corresponds
         * to the end-of-list specifier, as detailed in \ref command_line_parsing. */
        Argument (const char* name = nullptr, std::string description = std::string()) :
          id (name), desc (description), type (Text), flags (None) {
            defaults.text = nullptr;
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
          const char* text;
          struct {
            const char* const* list;
            int def;
          } choices;
          struct {
            int def, min, max;
          } i;
          struct {
            default_type def, min, max;
          } f;
        } defaults;


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
        /*! If desired, a default string can be specified using the \a
         * default_text argument. */
        Argument& type_text (const char* default_text = nullptr) {
          type = Text;
          defaults.text = default_text;
          return *this;
        }

        //! specifies that the argument should be an input image
        Argument& type_image_in () {
          type = ImageIn;
          defaults.text = nullptr;
          return *this;
        }

        //! specifies that the argument should be an output image
        Argument& type_image_out () {
          type = ImageOut;
          defaults.text = nullptr;
          return *this;
        }

        //! specifies that the argument should be an integer
        /*! if desired, a default value can be specified, along with a range of
         * allowed values. */
        Argument& type_integer (int min = std::numeric_limits<int>::min(), int def = 0, int max = std::numeric_limits<int>::max()) {
          type = Integer;
          defaults.i.min = min;
          defaults.i.def = def;
          defaults.i.max = max;
          return *this;
        }

        //! specifies that the argument should be a boolean
        /*! Valid responses are 0,no,false or any non-zero integer, yes, true. 
         * If desired, a default value can be specified. */
        Argument& type_bool (bool def = false) {
          type = Boolean;
          defaults.i.def = def;
          return *this;
        }

        //! specifies that the argument should be a floating-point value
        /*! if desired, a default value can be specified, along with a range of
         * allowed values. */
        Argument& type_float (default_type min = -std::numeric_limits<float>::infinity(), 
            default_type def = 0.0, default_type max = std::numeric_limits<default_type>::infinity()) {
          type = Float;
          defaults.f.min = min;
          defaults.f.def = def;
          defaults.f.max = max;
          return *this;
        }

        //! specifies that the argument should be selected from a predefined list
        /*! The list of allowed values must be specified as a nullptr-terminated
         * list of C strings.  If desired, a default value can be specified,
         * in the form of an index into the list. Here is an example usage:
         * \code
         * const char* mode_list [] = { "standard", "pedantic", "approx", nullptr };
         *
         * ARGUMENTS
         *   + Argument ("mode", "the mode of operation")
         *     .type_choice (mode_list, 0);
         * \endcode
         * \note Each string in the list must be supplied in \b lowercase. */
        Argument& type_choice (const char* const* choices, int default_index = -1) {
          type = Choice;
          defaults.choices.list = choices;
          defaults.choices.def = default_index;
          return *this;
        }

        //! specifies that the argument should be an input file
        Argument& type_file_in () {
          type = ArgFileIn;
          defaults.text = nullptr;
          return *this;
        }

        //! specifies that the argument should be an output file
        Argument& type_file_out () {
          type = ArgFileOut;
          defaults.text = nullptr;
          return *this;
        }

        //! specifies that the argument should be a sequence of comma-separated integer values
        Argument& type_sequence_int () {
          type = IntSeq;
          defaults.text = nullptr;
          return *this;
        }

        //! specifies that the argument should be a sequence of comma-separated floating-point values.
        Argument& type_sequence_float () {
          type = FloatSeq;
          defaults.text = nullptr;
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
    class Option : public std::vector<Argument> {
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
     */  
    class OptionGroup : public std::vector<Option> {
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
          return std::vector<Option>::back();
        }

        std::string header (int format) const;
        std::string contents (int format) const;
        static std::string footer (int format);
    };



  }

  //! @}
}

#endif
