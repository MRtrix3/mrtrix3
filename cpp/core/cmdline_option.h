/* Copyright (c) 2008-2025 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#pragma once

#include <cassert>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#ifdef None
#undef None
#endif

#include "mrtrix.h"
#include "types.h"
#include <variant>

namespace MR::App {

/*! \defgroup CmdParse Command-Line Parsing
 * \brief Classes and functions to parse command-line arguments and options.
 *
 * For a detailed description of the command-line parsing interface, see the
 * \ref command_line_parsing page.
 * */

using ArgTypeFlags = int;
constexpr ArgTypeFlags Undefined = 0;
constexpr ArgTypeFlags Text = 0x0001;
constexpr ArgTypeFlags Boolean = 0x0002;
constexpr ArgTypeFlags Integer = 0x0004;
constexpr ArgTypeFlags Float = 0x0008;
constexpr ArgTypeFlags ArgFileIn = 0x0010;
constexpr ArgTypeFlags ArgFileOut = 0x0020;
constexpr ArgTypeFlags ArgDirectoryIn = 0x0040;
constexpr ArgTypeFlags ArgDirectoryOut = 0x0080;
constexpr ArgTypeFlags ImageIn = 0x0100;
constexpr ArgTypeFlags ImageOut = 0x0200;
constexpr ArgTypeFlags IntSeq = 0x0400;
constexpr ArgTypeFlags FloatSeq = 0x0800;
constexpr ArgTypeFlags TracksIn = 0x1000;
constexpr ArgTypeFlags TracksOut = 0x2000;
constexpr ArgTypeFlags Choice = 0x4000;

using ArgModifierFlags = int;
constexpr ArgModifierFlags None = 0;
constexpr ArgModifierFlags Optional = 0x1;
constexpr ArgModifierFlags AllowMultiple = 0x2;
//! \endcond

namespace {
template <typename T> typename std::enable_if<std::is_integral<T>::value, T>::type void_rangemax() {
  return std::numeric_limits<T>::max();
}
template <typename T> typename std::enable_if<std::is_floating_point<T>::value, T>::type void_rangemax() {
  return std::numeric_limits<T>::infinity();
}
} // namespace

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
class Argument {
public:
  //! constructor
  /*! this is used to construct a command-line argument object, with a name
   * and description. If default arguments are used, the object corresponds
   * to the end-of-list specifier, as detailed in \ref command_line_parsing. */
  Argument(std::string name, std::string description = std::string())
      : id(std::move(name)), desc(std::move(description)), types(Undefined), flags(None) {}

  //! the argument name
  std::string id;
  //! the argument description
  std::string desc;
  //! the argument type(s)
  ArgTypeFlags types;
  //! the argument flags (AllowMultiple & Optional)
  ArgModifierFlags flags;

  std::vector<std::string> choices;

  template <typename T> class ScalarRange {
  public:
    ScalarRange() : _min(T(0)), _max(T(0)) {}
    operator bool() const { return _min != T(0) || _max != T(0); }
    void set(T i) {
      _min = i;
      _max = void_rangemax<T>();
    }
    void set(T i, T j) {
      _min = i;
      _max = j;
    }
    T min() const { return _min; }
    T max() const { return _max; }

  private:
    T _min, _max;
  };
  ScalarRange<int64_t> int_limits;
  ScalarRange<default_type> float_limits;

  operator bool() const { return !id.empty(); }

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
  Argument &optional() {
    flags |= Optional;
    return *this;
  }

  //! specifies that multiple such arguments can be specified
  /*! See optional() for details. */
  Argument &allow_multiple() {
    flags |= AllowMultiple;
    return *this;
  }

  //! specifies that the argument should be a text string
  Argument &type_text() {
    types |= Text;
    return *this;
  }

  //! specifies that the argument should be an input image
  Argument &type_image_in() {
    types |= ImageIn;
    return *this;
  }

  //! specifies that the argument should be an output image
  Argument &type_image_out() {
    types |= ImageOut;
    return *this;
  }

  //! specifies that the argument should be an integer
  /*! if desired, a range of allowed values can be specified. */
  Argument &type_integer(const int64_t min = std::numeric_limits<int64_t>::min(),
                         const int64_t max = std::numeric_limits<int64_t>::max()) {
    types |= Integer;
    int_limits.set(min, max);
    return *this;
  }

  //! specifies that the argument should be a boolean
  /*! Valid responses are 0,no,false or any non-zero integer, yes, true. */
  Argument &type_bool() {
    types |= Boolean;
    return *this;
  }

  //! specifies that the argument should be a floating-point value
  /*! if desired, a range of allowed values can be specified. */
  Argument &type_float(const default_type min = -std::numeric_limits<default_type>::infinity(),
                       const default_type max = std::numeric_limits<default_type>::infinity()) {
    types |= Float;
    float_limits.set(min, max);
    return *this;
  }

  //! specifies that the argument should be selected from a predefined list
  /*! The list of allowed values must be specified as a vector of strings.
   * Here is an example usage:
   * \code
   * const std::vector<std::string> mode_list = { "standard", "pedantic", "approx" };
   *
   * ARGUMENTS
   *   + Argument ("mode", "the mode of operation")
   *     .type_choice (mode_list);
   * \endcode
   * \note Each string in the list must be supplied in \b lowercase. */
  Argument &type_choice(const std::vector<std::string> &c) {
    types |= Choice;
    choices = c;
    return *this;
  }

  //! specifies that the argument should be an input file
  Argument &type_file_in() {
    types |= ArgFileIn;
    return *this;
  }

  //! specifies that the argument should be an output file
  Argument &type_file_out() {
    types |= ArgFileOut;
    return *this;
  }

  //! specifies that the argument should be an input directory
  Argument &type_directory_in() {
    types |= ArgDirectoryIn;
    return *this;
  }

  //! specifies that the argument should be an output directory
  Argument &type_directory_out() {
    types |= ArgDirectoryOut;
    return *this;
  }

  //! specifies that the argument should be a sequence of comma-separated integer values
  Argument &type_sequence_int() {
    types |= IntSeq;
    return *this;
  }

  //! specifies that the argument should be a sequence of comma-separated floating-point values.
  Argument &type_sequence_float() {
    types |= FloatSeq;
    return *this;
  }

  //! specifies that the argument should be an input tracks file
  Argument &type_tracks_in() {
    types |= TracksIn;
    return *this;
  }

  //! specifies that the argument should be an output tracks file
  Argument &type_tracks_out() {
    types |= TracksOut;
    return *this;
  }

  std::string syntax(int format) const;
  std::string usage() const;

  // According to how many different unique types can this argument be interpreted?
  ssize_t num_types() const {
    int data = types;
    ssize_t count;
    for (count = 0; data; ++count)
      data &= data - 1;
    return count;
  }
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
  Option() : flags(Optional) {}

  Option(std::string name, std::string description)
      : id(std::move(name)), desc(std::move(description)), flags(Optional) {}

  Option &operator+(const Argument &arg) {
    push_back(arg);
    return *this;
  }
  operator bool() const { return !id.empty(); }

  //! the option name
  std::string id;
  //! the option description
  std::string desc;
  //! option flags (AllowMultiple and/or Optional)
  ArgModifierFlags flags;

  //! specifies that the option is required
  /*! An option specified as required must be supplied on the command line.
   * For example:
   * \code
   * OPTIONS
   *   + Option ("roi",
   *       "the region of interest over which to perform the processing. "
   *       "Multiple such regions can be specified")
   *     .required()
   *     .allow_multiple()
   *     + Argument ("image").type_image_in();
   * \endcode
   */
  Option &required() {
    flags &= ~Optional;
    return (*this);
  }

  //! specifies that multiple such options can be specified
  /*! See required() for details. */
  Option &allow_multiple() {
    flags |= AllowMultiple;
    return *this;
  }

  bool is(const std::string &name) const { return name == id; }

  std::string syntax(int format) const;
  std::string usage() const;
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
class OptionGroup : public std::vector<Option> {
public:
  OptionGroup(std::string group_name = "OPTIONS") : name(std::move(group_name)) {}
  std::string name;

  OptionGroup &operator+(const Option &option) {
    push_back(option);
    return *this;
  }

  OptionGroup &operator+(const Argument &argument) {
    assert(!empty());
    back() + argument;
    return *this;
  }

  Option &back() {
    if (empty())
      push_back(Option());
    return std::vector<Option>::back();
  }

  std::string header(int format) const;
  std::string contents(int format) const;
  static std::string footer(int format);
};

} // namespace MR::App

//! @}
