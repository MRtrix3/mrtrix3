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

#include <algorithm>
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
//! render a floating-point value for help text, guaranteeing a decimal point for whole values
/*! Uses the standard MR::str() formatting (full double precision, matching the numeric range /
 * default rendering elsewhere), but appends ".0" when the result is a bare integer string (e.g.
 * "0" -> "0.0", "-5" -> "-5.0"), so that a floating-point limit or default is visually
 * distinguishable from an integer one. Values already carrying a decimal point, an exponent, or a
 * non-numeric form (inf / nan) are left untouched. */
inline std::string format_float(const default_type value) {
  std::string result = MR::str(value);
  const size_t first_digit = (!result.empty() && (result[0] == '-' || result[0] == '+')) ? 1 : 0;
  const bool integer_like =
      result.size() > first_digit && result.find_first_not_of("0123456789", first_digit) == std::string::npos;
  if (integer_like)
    result += ".0";
  return result;
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

  //! additional accepted spellings of individual choice values (alias -> canonical choice)
  /*! Each entry maps an alternative spelling of a choice value (e.g. an American spelling) to
   * one of the canonical (British) strings held in `choices`, so that both resolve to the same
   * underlying enum value. The alias key is stored lowercased and matched case-insensitively.
   * Only the canonical spelling is presented in the help text and machine-readable exports. */
  std::vector<std::pair<std::string, std::string>> choice_aliases;

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

  //! declare the default value applied when this argument / option is absent (integer convenience)
  template <typename T, typename std::enable_if<MR::is_integral<T>::value, int>::type = 0>
  Argument &set_default(const T value) {
    default_value = MR::str(value);
    return *this;
  }

  //! declare the default value applied when this argument / option is absent (floating-point convenience)
  /*! A whole-valued floating-point default renders with a trailing ".0" (via format_float()) so that
   * it is unambiguously floating-point, consistent with how floating-point ranges / limits render. */
  template <typename T, typename std::enable_if<MR::is_floating_point<T>::value, int>::type = 0>
  Argument &set_default(const T value) {
    default_value = format_float(value);
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

  //! register an alternative accepted spelling for one of this argument's choice values
  /*! `canonical_choice` must be one of the strings already present in `choices` (i.e.
   * type_choice<Enum>() must have been called first); `alias_spelling` is an additional
   * spelling (e.g. an American spelling) that resolves, case-insensitively, to that same
   * choice. The canonical spelling remains the only one shown in help / exports.
   * \code
   *   + Argument ("map").type_choice<ColourMap::Choice>().choice_alias("color", "colour");
   * \endcode */
  Argument &choice_alias(std::string alias_spelling, std::string canonical_choice) {
    assert(types[ArgTypeFlags::Choice]);
    assert(std::find(choices.begin(), choices.end(), canonical_choice) != choices.end());
    choice_aliases.emplace_back(MR::lowercase(alias_spelling), std::move(canonical_choice));
    return *this;
  }

  //! resolve a supplied token to its canonical choice spelling if it is a registered alias
  /*! Returns the canonical choice string when `token` (compared case-insensitively) matches a
   * spelling registered via choice_alias(); returns std::nullopt when it does not, leaving the
   * token to be validated against `choices` unchanged. */
  std::optional<std::string> resolve_choice_alias(std::string_view token) const {
    if (choice_aliases.empty())
      return std::nullopt;
    const std::string lowered = MR::lowercase(token);
    // Search for the (lowercased) alias entry and return its canonical choice: std::find_if.
    const auto entry = std::find_if(
        choice_aliases.begin(), choice_aliases.end(), [&lowered](const auto &e) { return e.first == lowered; });
    if (entry != choice_aliases.end())
      return entry->second;
    return std::nullopt;
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
  Argument &type_sequence_lmax() {
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

  //! the number of command-line tokens this (scalar) argument consumes
  /*! Always 1 for an Argument; provided so that Argument and ArgumentTuple present a
   * uniform interface to the ArgumentElement variant helpers (see element_arity()). */
  size_t arity() const { return 1; }

  //! the flattened scalar (leaf) arguments of this argument: just itself
  /*! Provided so that Argument and ArgumentTuple present a uniform interface to the
   * ArgumentElement variant helpers (see element_leaves()); the k-th leaf corresponds to
   * the k-th consumed token. */
  std::vector<const Argument *> leaves() const { return {this}; }

  //! the display representation of this argument's identifier: simply its id
  /*! Provided for interface uniformity with ArgumentTuple::syntax_id(), whose display id is
   * the space-joined ids of its sub-arguments. */
  std::string syntax_id() const { return id; }

  //! the auto-rendered annotation of permitted choices, numeric range and default value
  /*! Returns a string of parenthesised clauses, each preceded by a single space (e.g.
   * " (choices: a, b, c) (range: 0 to 1) (default: 0.5)"), or an empty string when this
   * argument carries no such metadata. Appended to the description by every human-readable
   * help surface so choices / ranges / defaults need not be written out by hand. */
  std::string help_metadata() const;

  std::string syntax(const bool format) const;
  std::string usage() const;
};

//! A class to specify a fixed-arity group of individually-typed command-line arguments
/*! An ArgumentTuple is a single, permutable command-line argument that consumes several
 * tokens at once, one per member Argument, each validated against that member's own type.
 * It is a first-class alternative to a plain Argument wherever a single logical argument is
 * naturally composed of several fields: a multi-argument option holds one ArgumentTuple
 * instead of several Arguments, and a positional slot may be an ArgumentTuple to accept
 * (optionally repeated) groups of fields (e.g. input/output image pairs).
 *
 * The tuple carries no type of its own; only its member Arguments are type-checked. Its
 * command-line syntax is derived (manifested) from the member argument names — its
 * syntax_id() is the space-joined member ids — so the usage line reads naturally without a
 * separate opaque metavar. Tuples do not nest (a member must be a scalar Argument).
 *
 * \code
 * ARGUMENTS
 *   + ArgumentTuple(Argument("response", "an input tissue response function").type_file_in(),
 *                   Argument("odf",      "the corresponding output ODF image").type_image_out())
 *     .set_description("pairs of input tissue response and output ODF images")
 *     .allow_multiple();
 *
 * OPTIONS
 *   + Option("fslgrad", "...")
 *     + ArgumentTuple(Argument("bvecs").type_file_in(), Argument("bvals").type_file_in());
 * \endcode */
class ArgumentTuple {
public:
  //! construct from two or more individually-typed member Arguments
  /*! The tuple manifests its syntax from these members' ids; each member must be a scalar
   * Argument (tuples do not nest). At least two members are required (a single-field tuple
   * would be an ordinary Argument). */
  template <
      typename... Rest,
      typename = std::enable_if_t<(sizeof...(Rest) >= 1) && (std::is_same<std::decay_t<Rest>, Argument>::value && ...)>>
  ArgumentTuple(Argument first, Rest... rest) {
    elements.push_back(std::move(first));
    (elements.push_back(std::move(rest)), ...);
  }

  //! construct from a pre-built list of member Arguments (must be non-empty; none a tuple)
  explicit ArgumentTuple(std::vector<Argument> members) : elements(std::move(members)) { assert(!elements.empty()); }

  //! the ordered, individually-typed member (field) arguments
  std::vector<Argument> elements;
  //! the group-level description rendered on a positional tuple's summary line (empty for options)
  /*! For a positional ArgumentTuple this describes the group as a whole (e.g. "pairs of input
   * response and output ODF images") and is rendered on the summary line alongside the
   * space-joined member ids; for an option tuple the option itself carries the description,
   * so this is left empty. */
  std::string desc;
  //! the tuple flags (Optional and/or AllowMultiple), applied to the group as a whole
  ArgModifierFlags flags;

  //! declare the group-level description of this (positional) tuple
  ArgumentTuple &set_description(std::string description) {
    desc = std::move(description);
    return *this;
  }

  //! specifies that the tuple (as a whole) is optional
  ArgumentTuple &optional() {
    flags.set_optional();
    return *this;
  }

  //! specifies that repeated groups of the tuple's fields may be supplied
  ArgumentTuple &allow_multiple() {
    flags.set_allow_multiple();
    return *this;
  }

  //! the number of command-line tokens the tuple consumes: one per member
  size_t arity() const { return elements.size(); }

  //! the flattened member (leaf) arguments; the k-th leaf corresponds to the k-th token
  std::vector<const Argument *> leaves() const {
    std::vector<const Argument *> result;
    result.reserve(elements.size());
    for (const auto &element : elements)
      result.push_back(&element);
    return result;
  }

  //! the display representation: the space-joined ids of the member arguments
  std::string syntax_id() const;

  //! the concatenated choice / range / default annotation of member fields that carry no description
  /*! A member with its own description renders its metadata on its own listing line; a member with
   * no description instead contributes its metadata here, so that — exactly as in the former
   * multi-scalar-argument option form — it is appended to the owning option's description line. */
  std::string help_metadata() const {
    std::string result;
    for (const auto &element : elements)
      if (element.desc.empty())
        result += element.help_metadata();
    return result;
  }

  std::string syntax(const bool format) const;
  std::string usage() const;
};

//! a single element of an ARGUMENTS list or of an Option: either a scalar Argument or an ArgumentTuple
using ArgumentElement = std::variant<Argument, ArgumentTuple>;

//! the number of command-line tokens the element consumes (1 for a scalar; the field count for a tuple)
inline size_t element_arity(const ArgumentElement &element) {
  return std::visit([](const auto &held) { return held.arity(); }, element);
}

//! the element's display id (the scalar's id, or the tuple's space-joined member ids)
inline std::string element_syntax_id(const ArgumentElement &element) {
  return std::visit([](const auto &held) { return held.syntax_id(); }, element);
}

//! the element's flags (Optional / AllowMultiple)
inline const ArgModifierFlags &element_flags(const ArgumentElement &element) {
  return std::visit([](const auto &held) -> const ArgModifierFlags & { return held.flags; }, element);
}

//! the flattened scalar (leaf) arguments of the element; the k-th leaf corresponds to the k-th token
inline std::vector<const Argument *> element_leaves(const ArgumentElement &element) {
  return std::visit([](const auto &held) { return held.leaves(); }, element);
}

//! the element's summary-line description (a scalar's description, or a tuple's group description)
inline std::string element_description(const ArgumentElement &element) {
  return std::visit([](const auto &held) { return held.desc; }, element);
}

//! the element's scalar choice / range / default annotation (empty for a tuple)
inline std::string element_help_metadata(const ArgumentElement &element) {
  return std::visit([](const auto &held) { return held.help_metadata(); }, element);
}

//! the tuple's member (field) arguments, or an empty list for a scalar element
inline const std::vector<Argument> &tuple_fields(const ArgumentElement &element) {
  static const std::vector<Argument> none;
  if (const auto *const tuple = std::get_if<ArgumentTuple>(&element))
    return tuple->elements;
  return none;
}

//! the element's terminal-help rendering (summary line plus, for a tuple, its member field lines)
inline std::string element_syntax(const ArgumentElement &element, const bool format) {
  return std::visit([format](const auto &held) { return held.syntax(format); }, element);
}

//! the element's __print_full_usage__ serialisation (one ARGUMENT line per leaf field)
inline std::string element_usage(const ArgumentElement &element) {
  return std::visit([](const auto &held) { return held.usage(); }, element);
}

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
class Option {
public:
  Option() { flags.set_optional(); }

  Option(std::string name, std::string description) : id(std::move(name)), desc(std::move(description)) {
    flags.set_optional();
  }

  //! the option's single argument item: a scalar Argument, an ArgumentTuple, or none (a flag)
  /*! An option accepts at most one item. A flag has no item (nullopt); a single-argument option
   * holds a scalar Argument; a multi-argument option holds one ArgumentTuple whose members are
   * the individual fields. The tokens consumed follow from this single item (see arity()). */
  std::optional<ArgumentElement> item;

  //! attach the option's sole scalar argument
  Option &operator+(const Argument &arg) {
    assert(!item.has_value());
    item = arg;
    return *this;
  }
  //! attach the option's sole argument tuple (a multi-field option)
  Option &operator+(const ArgumentTuple &tuple) {
    assert(!item.has_value());
    item = tuple;
    return *this;
  }
  operator bool() const { return !id.empty(); }

  //! the option name (canonical spelling)
  std::string id;
  //! the option description
  std::string desc;
  //! option flags (AllowMultiple and/or Optional)
  ArgModifierFlags flags;

  //! additional accepted spellings of this option (aliases)
  /*! The canonical `id` is unchanged and remains the sole spelling presented in the help
   * text and every machine-readable export. Any alias, and any unambiguous prefix of the
   * canonical id or of an alias, resolves to this option during command-line matching.
   * An alias must not introduce prefix-matching ambiguity with a *different* option. */
  std::vector<std::string> aliases;

  //! true if this option is a shared-IO-framework probe (see framework_probe())
  /*! Marks the small vocabulary of options that MRtrix's generic image / tractogram IO helpers
   * feature-test irrespective of whether the invoking command exposed them (e.g. a gradient
   * table embedded in an input image triggers a -grad probe even in a command offering no
   * gradient import). Set at the option's definition via framework_probe(); consulted, when the
   * option is absent from the executing command's interface, by App::is_framework_probe_option()
   * to exempt such a query from the registered-option-access invariant in App::get_options(). */
  bool framework_probe_flag = false;

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

  //! register an additional accepted spelling (alias) for this option
  /*! The canonical `id` is unchanged and remains the spelling shown in help / exports.
   * \code
   *   + Option ("normalise", "normalise the DW signal to the b=0 image").alias("normalize")
   * \endcode */
  Option &alias(std::string spelling) {
    aliases.push_back(std::move(spelling));
    return *this;
  }

  //! mark this option as a shared-IO-framework probe (see framework_probe_flag)
  /*! Records the flag on the option and registers the option's id in a process-wide set, so that
   * the property remains discoverable when the option is queried by a generic IO helper on a
   * command whose interface does not include it. Definition-side counterpart to the read-side
   * exemption applied by App::get_options(); the registry it populates is the sole source of the
   * exempt-id vocabulary, so it cannot drift from the option definitions themselves. */
  Option &framework_probe();

  //! true if `name` matches the canonical id or any alias exactly
  bool is(std::string_view name) const {
    // Exact-equality search over the alias spellings: std::find expresses precisely that.
    return name == id || std::find(aliases.begin(), aliases.end(), name) != aliases.end();
  }

  //! true if `stub` is a prefix of the canonical id or of any alias
  /*! A spelling S is prefixed by `stub` when S begins with `stub` (and is at least as long).
   * Because both spellings of an option answer to the same Option object, a prefix shared by
   * the canonical id and an alias resolves to this single option rather than being ambiguous. */
  bool matches_prefix(std::string_view stub) const {
    const auto is_prefix_of = [stub](std::string_view spelling) {
      return stub.size() <= spelling.size() && spelling.substr(0, stub.size()) == stub;
    };
    // Predicate search over the alias spellings: std::any_of expresses precisely that.
    return is_prefix_of(id) || std::any_of(aliases.begin(), aliases.end(), is_prefix_of);
  }

  //! the total number of command-line tokens consumed by this option's argument item
  /*! Zero for a flag, one for a scalar argument, or the field count for an argument tuple.
   * This is the count of tokens the parser consumes immediately following the option. */
  size_t arity() const { return item.has_value() ? element_arity(*item) : 0; }

  //! the flattened list of scalar (leaf) sub-arguments of this option
  /*! An argument tuple is expanded into its member fields; a scalar argument maps to itself; a
   * flag yields none. The k-th leaf corresponds to the k-th command-line token consumed by the
   * option, i.e. to ParsedOption::operator[](k). */
  std::vector<const Argument *> leaves() const {
    return item.has_value() ? element_leaves(*item) : std::vector<const Argument *>();
  }

  //! the auto-rendered choice / range / default annotation of this option's scalar argument
  /*! The option's scalar argument metadata (choices / range / default) appended to its
   * description automatically; empty for a flag or for a tuple item (whose member fields render
   * their own metadata on their listing lines). */
  std::string help_metadata() const { return item.has_value() ? element_help_metadata(*item) : std::string(); }

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

  OptionGroup &operator+(const ArgumentTuple &tuple) {
    assert(!empty());
    back() + tuple;
    return *this;
  }

  //! nest a child group within this group
  OptionGroup &operator+(const OptionGroup &subgroup) {
    subgroups.push_back(subgroup);
    return *this;
  }

  //! require that exactly one of this group's member options is specified
  OptionGroup &require_exactly_one() { return set_constraint(Constraint::RequireExactlyOne); }

  //! require that at least one of this group's member options is specified
  OptionGroup &require_at_least_one() { return set_constraint(Constraint::RequireAtLeastOne); }

  //! permit at most one of this group's member options to be specified (all mutually exclusive)
  OptionGroup &mutually_exclusive() { return set_constraint(Constraint::MutuallyExclusive); }

  //! require that this group's member options are either all specified or none specified
  OptionGroup &all_or_none() { return set_constraint(Constraint::AllOrNone); }

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

  //! the greatest nesting depth of any sub-group beneath this group (0 if it has no sub-groups)
  /*! A direct child sub-group is depth 1, a grandchild depth 2, etc. Used by the documentation
   * exporters to decide whether the deepest groups still fit within the finite heading capacity
   * of the Markdown / reStructuredText formats, or must degrade to emphasis-based rendering. */
  size_t max_subgroup_depth() const {
    size_t result = 0;
    for (const auto &subgroup : subgroups)
      result = std::max(result, size_t(1) + subgroup.max_subgroup_depth());
    return result;
  }

  //! the auto-generated help annotation describing this group's collective constraint
  /*! Returns a parenthesised note whose wording matches the corresponding parse-time error
   *  message (e.g. "(exactly one of these options must be specified)" for RequireExactlyOne),
   *  or an empty string when the group carries no constraint (Constraint::None). Every
   *  human-readable help surface (the terminal -help and the Markdown / reStructuredText
   *  exports) renders it beneath the group's options, so a declared constraint need not be
   *  restated by hand in any option description or group heading. */
  std::string constraint_annotation() const;

  //! the section heading for this group; depth controls the indentation / heading level
  std::string header(const bool format, const size_t depth = 0) const;
  //! this group's options followed by its sub-groups (each headed and rendered recursively)
  std::string contents(const bool format, const size_t depth = 0) const;
  static std::string footer(const bool format);

private:
  //! apply a single collective constraint, rejecting any second constraint modifier on this group
  /*! The group holds exactly one Constraint value, so a second constraint-modification method
   *  (require_exactly_one() / require_at_least_one() / mutually_exclusive() / all_or_none()) would
   *  silently override the first — a command-author error. Because the modifiers all return
   *  OptionGroup& to support builder chaining and the group is stored as a plain OptionGroup in the
   *  OPTIONS builder, a second application cannot be rejected at compile time without a type-state
   *  redesign of the whole builder API; it is instead caught here during command-interface
   *  construction (usage()) and fails fast with a hierarchical exception naming the group and both
   *  constraints. This fires in every build (unlike an assert stripped under NDEBUG). */
  OptionGroup &set_constraint(const Constraint requested) {
    if (constraint != Constraint::None)
      throw Exception(std::string("option group \"") + name + "\" already carries constraint " +
                      MR::Enum::name(constraint) + "; cannot additionally apply constraint " +
                      MR::Enum::name(requested) +
                      " (at most one constraint modifier may be applied to any one option group)");
    constraint = requested;
    return *this;
  }
};

} // namespace MR::App

//! @}
