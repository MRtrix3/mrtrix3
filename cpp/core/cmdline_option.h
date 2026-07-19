/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include <bitset>
#include <cassert>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "enum.h"
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

class ArgTypeFlags : public std::bitset<17> {
public:
  ArgTypeFlags() = default;
  inline static constexpr ssize_t Text = 0;
  inline static constexpr ssize_t Boolean = 1;
  inline static constexpr ssize_t Integer = 2;
  inline static constexpr ssize_t Float = 3;
  inline static constexpr ssize_t FileIn = 4;
  inline static constexpr ssize_t FileOut = 5;
  inline static constexpr ssize_t DirectoryIn = 6;
  inline static constexpr ssize_t DirectoryOut = 7;
  inline static constexpr ssize_t ImageIn = 8;
  inline static constexpr ssize_t ImageOut = 9;
  inline static constexpr ssize_t IntSeq = 10;
  inline static constexpr ssize_t FloatSeq = 11;
  inline static constexpr ssize_t TracksIn = 12;
  inline static constexpr ssize_t TracksOut = 13;
  inline static constexpr ssize_t Choice = 14;
  //! a single maximal spherical harmonic degree (lmax): a non-negative even integer
  /*! A refinement of Integer: the Integer bit is set alongside Lmax so that all existing
   * integer machinery (parsing, range display, readers) applies unchanged, while the Lmax
   * bit triggers the additional non-negative-and-even validation at parse time. */
  inline static constexpr ssize_t Lmax = 15;
  //! a comma-separated list of maximal spherical harmonic degrees (lmax), each non-negative and even
  /*! A refinement of IntSeq: the IntSeq bit is set alongside LmaxSeq, so the sequence parsing
   * is shared while the LmaxSeq bit triggers per-element non-negative-and-even validation. */
  inline static constexpr ssize_t LmaxSeq = 16;
};

class ArgModifierFlags {
public:
  ArgModifierFlags() = default;
  ArgModifierFlags(const ArgModifierFlags &) = default;
  ~ArgModifierFlags() = default;
  ArgModifierFlags &operator=(const ArgModifierFlags &) = default;
  void set_optional() { data.set(Optional); }
  void set_required() { data.reset(Optional); }
  void set_allow_multiple() { data.set(AllowMultiple); }
  bool optional() const { return data[Optional]; }
  bool required() const { return !data[Optional]; }
  bool allow_multiple() const { return data[AllowMultiple]; }
  bool any() const { return data.any(); }
  bool operator!=(const ArgModifierFlags &that) const { return data != that.data; }

private:
  std::bitset<2> data;
  inline static constexpr ssize_t Optional = 0;
  inline static constexpr ssize_t AllowMultiple = 1;
};

//! \endcond

namespace {
template <typename T> typename std::enable_if<MR::is_integral<T>::value, T>::type void_rangemax() {
  return std::numeric_limits<T>::max();
}
template <typename T> typename std::enable_if<MR::is_floating_point<T>::value, T>::type void_rangemax() {
  return std::numeric_limits<T>::infinity();
}
template <typename T> typename std::enable_if<MR::is_integral<T>::value, T>::type void_rangemin() {
  return std::numeric_limits<T>::min();
}
template <typename T> typename std::enable_if<MR::is_floating_point<T>::value, T>::type void_rangemin() {
  return -std::numeric_limits<T>::infinity();
}
} // namespace

//! \addtogroup CmdParse
// @{

//! Specifies how a directory output argument should be validated with respect to pre-existence
/*! Used as the parameter to Argument::type_directory_out() to control what parse-time check
 *  is applied when the specified path already exists. */
enum class DirOutMode {
  MustNotExist,  //!< Directory must not already exist; -force overrides
  EmptyOrAbsent, //!< Directory must not exist, or exist but be empty; -force does not override
  MayExist,      //!< No pre-existence check; the command is responsible for creation and access
};

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
      : id(std::move(name)), desc(std::move(description)) {}

  //! the argument name
  std::string id;
  //! the argument description
  std::string desc;
  //! the argument type(s)
  ArgTypeFlags types;
  //! the argument flags (AllowMultiple & Optional)
  ArgModifierFlags flags;

  std::vector<std::string> choices;
  //! for DirectoryOut arguments, specifies behaviour with respect to pre-existing directories
  DirOutMode dir_out_mode = DirOutMode::MustNotExist;

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
    //! true if a lower bound other than the type's unbounded sentinel has been set
    bool has_min() const { return _min != void_rangemin<T>(); }
    //! true if an upper bound other than the type's unbounded sentinel has been set
    bool has_max() const { return _max != void_rangemax<T>(); }

  private:
    T _min, _max;
  };
  ScalarRange<int64_t> int_limits;
  ScalarRange<default_type> float_limits;

  //! the default value applied by the command when this argument / option is not supplied
  /*! Held as free text so it may describe non-scalar defaults (e.g. "3x3x3", "no limit").
   * When set, the value is auto-rendered as "(default: <value>)" in the terminal help and
   * every machine-readable export, removing the need to repeat it by hand in the description. */
  std::optional<std::string> default_value;

  operator bool() const { return !id.empty(); }

  //! declare the default value applied when this argument / option is absent (pre-formatted text)
  Argument &set_default(std::string value) {
    default_value = std::move(value);
    return *this;
  }

  //! declare the default value applied when this argument / option is absent (numeric convenience)
  template <typename T, typename std::enable_if<MR::is_arithmetic<T>::value, int>::type = 0>
  Argument &set_default(const T value) {
    default_value = MR::str(value);
    return *this;
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
  Argument &optional() {
    flags.set_optional();
    return *this;
  }

  //! specifies that multiple such arguments can be specified
  /*! See optional() for details. */
  Argument &allow_multiple() {
    flags.set_allow_multiple();
    return *this;
  }

  //! specifies that the argument should be a text string
  Argument &type_text() {
    types.set(ArgTypeFlags::Text);
    return *this;
  }

  //! specifies that the argument should be an input image
  Argument &type_image_in() {
    types.set(ArgTypeFlags::ImageIn);
    return *this;
  }

  //! specifies that the argument should be an output image
  Argument &type_image_out() {
    types.set(ArgTypeFlags::ImageOut);
    return *this;
  }

  //! specifies that the argument should be an integer
  /*! if desired, a range of allowed values can be specified. */
  Argument &type_integer(const int64_t min = std::numeric_limits<int64_t>::min(),
                         const int64_t max = std::numeric_limits<int64_t>::max()) {
    types.set(ArgTypeFlags::Integer);
    int_limits.set(min, max);
    return *this;
  }

  //! specifies that the argument should be a boolean
  /*! Valid responses are 0,no,false or any non-zero integer, yes, true. */
  Argument &type_bool() {
    types.set(ArgTypeFlags::Boolean);
    return *this;
  }

  //! specifies that the argument should be a floating-point value
  /*! if desired, a range of allowed values can be specified. */
  Argument &type_float(const default_type min = -std::numeric_limits<default_type>::infinity(),
                       const default_type max = std::numeric_limits<default_type>::infinity()) {
    types.set(ArgTypeFlags::Float);
    float_limits.set(min, max);
    return *this;
  }

  //! specifies that the argument should be selected from a predefined list of enum values
  /*! The list of allowed values is automatically generated from the enum type provided as template parameter.
   * Here is an example usage:
   * \code
   * enum class Mode { Standard, Pedantic, Approx };
   *
   * ARGUMENTS
   *   + Argument ("mode", "the mode of operation")
   *     .type_choice<Mode>();
   * \endcode
   * Enumerators that are meaningful internally but should not be user-selectable can be
   * omitted from the presented set by excluding them from the enum's magic_enum reflection
   * (a magic_enum::customize::enum_name specialization returning invalid_tag, declared at the
   * enum definition); such values are then absent from the choices here, and are rejected by
   * MR::Enum::from_name() / App::get_option_choice() when supplied on the command-line.
   * \note Each enum value in the list must be supplied in \b lowercase. */
  template <typename Enum> Argument &type_choice() {
    static_assert(std::is_enum_v<Enum>, "Template parameter must be an enum type");
    types.set(ArgTypeFlags::Choice);
    choices = MR::Enum::lower_case_names<Enum>();
    return *this;
  }

  //! specifies that the argument should be an input file
  Argument &type_file_in() {
    types.set(ArgTypeFlags::FileIn);
    return *this;
  }

  //! specifies that the argument should be an output file
  Argument &type_file_out() {
    types.set(ArgTypeFlags::FileOut);
    return *this;
  }

  //! specifies that the argument should be an input directory
  Argument &type_directory_in() {
    types.set(ArgTypeFlags::DirectoryIn);
    return *this;
  }

  //! specifies that the argument should be an output directory
  /*! \param mode controls how pre-existence of the path is handled at parse time. */
  Argument &type_directory_out(DirOutMode mode) {
    types.set(ArgTypeFlags::DirectoryOut);
    dir_out_mode = mode;
    return *this;
  }

  //! specifies that the argument should be a sequence of comma-separated integer values
  Argument &type_sequence_int() {
    types.set(ArgTypeFlags::IntSeq);
    return *this;
  }

  //! specifies that the argument should be a sequence of comma-separated floating-point values.
  Argument &type_sequence_float() {
    types.set(ArgTypeFlags::FloatSeq);
    return *this;
  }

  //! specifies that the argument should be a maximal spherical harmonic degree (lmax)
  /*! An lmax is a non-negative even integer; both constraints are enforced at parse time
   * (see App::ParsedArgument::as_int()) and surfaced automatically in the help. The optional
   * bounds constrain the permitted magnitude further (e.g. a command may require a minimum of
   * 2, or impose a sanity upper bound); the default lower bound of 0 encodes non-negativity. */
  Argument &type_lmax(const int64_t min = 0, const int64_t max = std::numeric_limits<int64_t>::max()) {
    types.set(ArgTypeFlags::Integer);
    types.set(ArgTypeFlags::Lmax);
    int_limits.set(min, max);
    return *this;
  }

  //! specifies that the argument should be a comma-separated list of lmax values
  /*! Each element is a non-negative even integer; both constraints are enforced per element at
   * parse time (see App::ParsedArgument::as_sequence_int() / as_sequence_uint()) and surfaced
   * automatically in the help. Command-specific validation of the list (e.g. matching the number
   * of entries to the number of shells or tissues, or clamping to a per-shell upper bound) remains
   * the responsibility of the individual command. */
  Argument &type_lmax_sequence() {
    types.set(ArgTypeFlags::IntSeq);
    types.set(ArgTypeFlags::LmaxSeq);
    return *this;
  }

  //! specifies that the argument should be an input tracks file
  Argument &type_tracks_in() {
    types.set(ArgTypeFlags::TracksIn);
    return *this;
  }

  //! specifies that the argument should be an output tracks file
  Argument &type_tracks_out() {
    types.set(ArgTypeFlags::TracksOut);
    return *this;
  }

  //! for a tuple argument, the ordered list of typed sub-arguments
  /*! When non-empty, this Argument is a "tuple": a single logical, permutable
   * command-line argument that consumes elements.size() tokens, one per sub-argument,
   * each validated against that sub-argument's type. Each sub-argument carries its own
   * id, description and type. A tuple must not itself contain a tuple (one level only). */
  std::vector<Argument> elements;

  //! true if this argument is a tuple (an ordered group of sub-arguments)
  bool is_tuple() const { return !elements.empty(); }

  //! the number of command-line tokens this argument consumes
  /*! 1 for a scalar argument; the number of sub-arguments for a tuple. */
  size_t arity() const { return elements.empty() ? size_t(1) : elements.size(); }

  //! specifies that the argument is a tuple of ordered, individually-typed sub-arguments
  /*! Each sub-argument is provided with its own id, description and type. The tuple is
   * consumed as a single logical argument of fixed arity (equal to the number of
   * sub-arguments) that remains permutable with the other arguments and options. A tuple
   * positional argument may be declared .allow_multiple() to accept repeated groups of
   * sub-arguments (e.g. repeated input/output pairs).
   * \note sub-arguments must be scalar; a tuple cannot itself contain a tuple. */
  Argument &type_tuple(std::vector<Argument> subarguments) {
    assert(!subarguments.empty());
    for (const auto &element : subarguments)
      assert(!element.is_tuple());
    elements = std::move(subarguments);
    return *this;
  }

  //! the display representation of this argument's identifier(s)
  /*! For a scalar argument this is simply its id; for a tuple it is the space-joined
   * ids of its sub-arguments, so that the command-line syntax lines read naturally. */
  std::string syntax_id() const;

  //! the auto-rendered annotation of permitted choices, numeric range and default value
  /*! Returns a string of parenthesised clauses, each preceded by a single space (e.g.
   * " (choices: a, b, c) (range: 0 to 1) (default: 0.5)"), or an empty string when this
   * argument carries no such metadata. Appended to the description by every human-readable
   * help surface so choices / ranges / defaults need not be written out by hand. */
  std::string help_metadata() const;

  std::string syntax(const bool format) const;
  std::string usage() const;
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
  Option() { flags.set_optional(); }

  Option(std::string name, std::string description) : id(std::move(name)), desc(std::move(description)) {
    flags.set_optional();
  }

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
    flags.set_required();
    return (*this);
  }

  //! specifies that multiple such options can be specified
  /*! See required() for details. */
  Option &allow_multiple() {
    flags.set_allow_multiple();
    return *this;
  }

  bool is(std::string_view name) const { return name == id; }

  //! the total number of command-line tokens consumed by this option's arguments
  /*! Equal to the number of arguments for an option of scalar arguments; a tuple
   * argument contributes as many tokens as it has sub-arguments. This is the count of
   * tokens the parser consumes immediately following the option on the command-line. */
  size_t arity() const {
    size_t total = 0;
    for (const auto &arg : *this)
      total += arg.arity();
    return total;
  }

  //! the flattened list of scalar (leaf) sub-arguments of this option
  /*! Tuple arguments are expanded into their constituent sub-arguments; scalar arguments
   * map to themselves. The k-th leaf corresponds to the k-th command-line token consumed
   * by the option, i.e. to ParsedOption::operator[](k). */
  std::vector<const Argument *> leaves() const {
    std::vector<const Argument *> result;
    result.reserve(arity());
    for (const auto &arg : *this) {
      if (arg.is_tuple())
        for (const auto &element : arg.elements)
          result.push_back(&element);
      else
        result.push_back(&arg);
    }
    return result;
  }

  //! the auto-rendered choice / range / default annotation of this option's scalar arguments
  /*! Concatenates the help_metadata() of each non-tuple argument, so that an option's permitted
   * choices, numeric range and default value are appended to its description automatically.
   * Tuple sub-arguments are excluded here; their metadata is rendered on their own listing lines. */
  std::string help_metadata() const;

  std::string syntax(const bool format) const;
  std::string usage() const;
};

//! a class to hold a named list of Option's, optionally with nested child groups
/*! the name is used as the section heading for the options that follow. A group may in
 * addition contain nested child groups (sub-groups), forming a hierarchy of option
 * sections; a child group is appended with the same operator+() idiom used for options,
 * the operand being an OptionGroup rather than an Option. Each surface (terminal help and
 * the machine-readable exports) conveys the nesting by increasing depth (indentation /
 * heading level). A group's own direct options are always rendered before its sub-groups.
 * For example:
 * \code
 * void usage () {
 *   ...
 *   OPTIONS
 *   + Option (...)
 *
 *   + OptionGroup ("Special options")
 *   + Option ("option1", ...)
 *   + Option ("option2", ...)
 *
 *   + (OptionGroup ("Parent group")
 *      + Option ("direct_option", ...)
 *      + (OptionGroup ("Nested child group")
 *         + Option ("nested_option", ...)));
 * }
 * \endcode
 */
class OptionGroup : public std::vector<Option> {
public:
  //! the parse-time constraint a group can impose collectively on its member options
  /*! A constraint is evaluated over every option in the group and, recursively, its
   *  sub-groups (i.e. over all_options()); "specified" means the option appears at least
   *  once on the command-line. The default is None (no constraint). Enforcement occurs
   *  during App::parse(), before any input file is accessed. */
  enum class Constraint {
    None,              //!< no collective constraint (default)
    RequireExactlyOne, //!< exactly one member option must be specified
    RequireAtLeastOne, //!< at least one member option must be specified
    MutuallyExclusive, //!< at most one member option may be specified
    AllOrNone,         //!< either every member option is specified, or none of them is
  };

  OptionGroup(std::string group_name = "OPTIONS") : name(std::move(group_name)) {}
  std::string name;

  //! the collective constraint imposed on this group's member options (see Constraint)
  Constraint constraint = Constraint::None;

  //! nested child groups (empty for a flat group)
  std::vector<OptionGroup> subgroups;

  OptionGroup &operator+(const Option &option) {
    push_back(option);
    return *this;
  }

  OptionGroup &operator+(const Argument &argument) {
    assert(!empty());
    back() + argument;
    return *this;
  }

  //! nest a child group within this group
  OptionGroup &operator+(const OptionGroup &subgroup) {
    subgroups.push_back(subgroup);
    return *this;
  }

  //! require that exactly one of this group's member options is specified
  OptionGroup &require_exactly_one() {
    constraint = Constraint::RequireExactlyOne;
    return *this;
  }

  //! require that at least one of this group's member options is specified
  OptionGroup &require_at_least_one() {
    constraint = Constraint::RequireAtLeastOne;
    return *this;
  }

  //! permit at most one of this group's member options to be specified (all mutually exclusive)
  OptionGroup &mutually_exclusive() {
    constraint = Constraint::MutuallyExclusive;
    return *this;
  }

  //! require that this group's member options are either all specified or none specified
  OptionGroup &all_or_none() {
    constraint = Constraint::AllOrNone;
    return *this;
  }

  Option &back() {
    if (empty())
      push_back(Option());
    return std::vector<Option>::back();
  }

  //! the flattened list of all options in this group and, recursively, its sub-groups
  /*! Own direct options first, then those of each sub-group in order. The returned pointers
   * address the options in place, so they retain the identity used by the parser / matchers. */
  std::vector<const Option *> all_options() const {
    std::vector<const Option *> result;
    for (const auto &option : *this)
      result.push_back(&option);
    for (const auto &subgroup : subgroups) {
      const std::vector<const Option *> nested = subgroup.all_options();
      result.insert(result.end(), nested.begin(), nested.end());
    }
    return result;
  }

  //! recursively locate an option by exact id within this group or its sub-groups
  const Option *find(std::string_view option_name) const {
    for (const auto &option : *this) {
      if (option.is(option_name))
        return &option;
    }
    for (const auto &subgroup : subgroups) {
      if (const Option *const match = subgroup.find(option_name))
        return match;
    }
    return nullptr;
  }

  //! true if the given option pointer refers to an option in this group or a sub-group
  bool contains(const Option *ptr) const {
    for (const auto &option : *this) {
      if (&option == ptr)
        return true;
    }
    for (const auto &subgroup : subgroups) {
      if (subgroup.contains(ptr))
        return true;
    }
    return false;
  }

  //! the section heading for this group; depth controls the indentation / heading level
  std::string header(const bool format, const size_t depth = 0) const;
  //! this group's options followed by its sub-groups (each headed and rendered recursively)
  std::string contents(const bool format, const size_t depth = 0) const;
  static std::string footer(const bool format);
};

} // namespace MR::App

//! @}
