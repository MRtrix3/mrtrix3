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

#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>

#ifdef None
#undef None
#endif

#include "cmdline_option.h"
#include "enum.h"
#include "file/path.h"
#include "types.h"

extern void usage();
extern void run();

namespace MR::App {

extern const std::string mrtrix_version;
extern const std::string build_date;
extern int log_level;
extern int exit_error_code;
extern std::string NAME;
extern std::string command_history_string;
extern bool overwrite_files;
extern void (*check_overwrite_files_func)(const std::filesystem::path &name);
extern bool fail_on_warn;
extern bool terminal_use_colour;
extern const std::thread::id main_thread_ID;

extern std::vector<std::string> raw_arguments_list;

extern const std::string project_version;
extern const std::string project_build_date;

struct HelpFormatting {
  struct Indents {
    ssize_t header;
    ssize_t main;
  };
  const ssize_t width;
  const Indents purpose_indents;
  const Indents arg_indents;
  const Indents option_indents;
  const ssize_t example_indent;
};

extern const HelpFormatting help_formatting;

extern const std::string help_command;

extern const std::string core_reference;

std::string help_head(const bool format);
std::string help_synopsis(const bool format);
std::string help_tail(const bool format);
std::string usage_syntax(const bool format);

//! \addtogroup CmdParse
// @{

//! vector of strings to hold more comprehensive command description
class Description : public std::vector<std::string> {
public:
  Description &operator+(const char *text); // check_syntax off
  Description &operator+(std::string_view text);
  Description &operator+(const char *const text[]); // check_syntax off

  std::string syntax(const bool format) const;
};

//! object for storing a single example command usage
class Example {
public:
  Example(std::string_view title, std::string_view code, std::string_view description);
  // Non-const so that ExampleList (and hence Subcommand) is copy-assignable, as required
  //   by the hierarchical-command machinery; treat as immutable after construction.
  std::string title;
  std::string code;
  std::string description;

  operator std::string() const;
  std::string syntax(const bool format) const;
};

//! a class to hold the list of Example's
class ExampleList : public std::vector<Example> {
public:
  ExampleList &operator+(const Example &example);

  std::string syntax(const bool format) const;
};

//! a class to hold the list of Argument's
class ArgumentList : public std::vector<Argument> {
public:
  ArgumentList &operator+(const Argument &argument);

  std::string syntax(const bool format) const;
};

//! a class to hold the list of option groups
class OptionList : public std::vector<OptionGroup> {
public:
  OptionList &operator+(const OptionGroup &option_group);

  OptionList &operator+(const Option &option);

  OptionList &operator+(const Argument &argument);

  OptionGroup &back();

  std::string syntax(const bool format) const;
};

//! A single sub-interface of a hierarchical command (a mode / algorithm / operation)
/*! A hierarchical command dispatches to one of several sub-interfaces selected by the
 *  first positional command-line token (see \ref command_line_parsing). Each Subcommand
 *  carries its own synopsis, positional arguments, options, examples and references, in
 *  the same manner as an ordinary command's usage() globals. The command declares an
 *  ordered list of these in the SUBCOMMANDS global; the selected sub-interface is then
 *  merged with the command's common options (those declared in OPTIONS) and the standard
 *  options. Options preceding the selection token are interpreted by the selected
 *  sub-interface, i.e. they are permutable with, and reachable across, the selection.
 *
 *  The builder idiom mirrors the global usage() blocks, e.g.:
 *  \code
 *  SUBCOMMANDS_SELECTOR = "operation";
 *  SUBCOMMANDS
 *  + Subcommand("import")
 *      .set_synopsis("Import external data")
 *      .set_arguments(ArgumentList()
 *        + Argument("in", "the input").type_file_in()
 *        + Argument("out", "the output").type_file_out())
 *      .set_options(OptionList()
 *        + Option("scale", "scale factor")
 *          + Argument("value").type_float());
 *  \endcode */
class Subcommand {
public:
  explicit Subcommand(std::string name) : id(std::move(name)) {}

  //! the selection token (matched exactly; no prefix abbreviation)
  std::string id;
  //! one-sentence synopsis of this sub-interface
  std::string synopsis;
  //! author of this sub-interface (falls back to the command's AUTHOR when empty)
  std::string author;
  //! copyright of this sub-interface (falls back to the command's COPYRIGHT when empty)
  std::string copyright;
  //! extended description of this sub-interface
  Description description;
  //! example usages of this sub-interface
  ExampleList examples;
  //! positional arguments of this sub-interface
  ArgumentList arguments;
  //! options specific to this sub-interface
  OptionList options;
  //! references specific to this sub-interface (appended to the command-level REFERENCES)
  Description references;

  Subcommand &set_synopsis(std::string text) {
    synopsis = std::move(text);
    return *this;
  }
  Subcommand &set_author(std::string text) {
    author = std::move(text);
    return *this;
  }
  Subcommand &set_copyright(std::string text) {
    copyright = std::move(text);
    return *this;
  }
  Subcommand &set_description(Description text) {
    description = std::move(text);
    return *this;
  }
  Subcommand &set_examples(ExampleList list) {
    examples = std::move(list);
    return *this;
  }
  Subcommand &set_arguments(ArgumentList list) {
    arguments = std::move(list);
    return *this;
  }
  Subcommand &set_options(OptionList list) {
    options = std::move(list);
    return *this;
  }
  Subcommand &set_references(Description text) {
    references = std::move(text);
    return *this;
  }

  bool is(std::string_view name) const { return name == id; }
};

//! an ordered list of a hierarchical command's Subcommand sub-interfaces
class SubcommandList : public std::vector<Subcommand> {
public:
  SubcommandList &operator+(const Subcommand &sub) {
    push_back(sub);
    return *this;
  }

  //! locate a sub-interface by exact id, or nullptr if none matches
  const Subcommand *find(std::string_view name) const {
    for (const auto &sub : *this)
      if (sub.is(name))
        return &sub;
    return nullptr;
  }

  //! the ordered list of sub-interface ids (declaration order)
  std::vector<std::string> ids() const {
    std::vector<std::string> result;
    result.reserve(size());
    for (const auto &sub : *this)
      result.push_back(sub.id);
    return result;
  }
};

void check_overwrite(const std::filesystem::path &path);

//! initialise MRtrix and parse command-line arguments
/*! this function must be called from within main(), immediately after the
 * argument and options have been specified, and before any further
 * processing takes place. */
void init(int argc, const char *const *argv); // check_syntax off

//! verify that command's usage() function has set requisite fields [used internally]
void verify_usage();

//! option parsing that should happen before GUI creation [used internally]
void parse_special_options();

//! do the actual parsing of the command-line [used internally]
void parse();

//! sort command-line tokens into arguments and options [used internally]
void sort_arguments(const std::vector<std::string> &arguments);

//! uniquely match option stub to Option
const Option *match_option(std::string_view arg);

//! dump formatted help page [used internally]
std::string full_usage();

class ParsedArgument {
public:
  using IntType = int64_t; // Native single-integer parsed type before conversion
  using UIntType = std::make_unsigned_t<IntType>;

  operator std::string() const;
  operator std::string_view() const;
  operator std::filesystem::path() const;

  // This particular function is permissive of reading the argument in this form
  //   even if the argument is not explicitly flagged as being of text type;
  //   in particular, attempting to implicitly convert to std::string
  //   for an argument that is a filesystem path type will fail,
  //   whereas this function can be used
  std::string as_text() const { return p; }

  std::filesystem::path as_path() const;
  bool as_bool() const;
  IntType as_int() const;
  UIntType as_uint() const;
  default_type as_float() const;

  std::vector<IntType> as_sequence_int() const;
  std::vector<UIntType> as_sequence_uint() const;
  std::vector<default_type> as_sequence_float() const;

  operator bool() const { return as_bool(); }
  operator int() const { return as_int(); }
  operator unsigned int() const { return as_uint(); }
  operator long int() const { return as_int(); }
  operator long unsigned int() const { return as_uint(); }
  operator long long int() const { return as_int(); }
  operator long long unsigned int() const { return as_uint(); }
  operator float() const { return as_float(); }
  operator double() const { return as_float(); }
  operator std::vector<IntType>() const { return as_sequence_int(); }
  operator std::vector<UIntType>() const { return as_sequence_uint(); }
  operator std::vector<default_type>() const { return as_sequence_float(); }

  const char *c_str() const { return p.c_str(); } // check_syntax off

  //! the index of this argument in the raw command-line arguments list
  size_t index() const { return index_; }

private:
  const Option *opt;
  const Argument *arg;
  std::string p;
  size_t index_;

  bool includes_filesystem_arg_types() const noexcept;
  bool only_filesystem_arg_types() const noexcept;

  ParsedArgument(const Option *option, const Argument *argument, std::string text, size_t index);

  friend class ParsedOption;
  friend class Options;
  friend void MR::App::init(int argc, const char *const *argv); // check_syntax off
  friend void MR::App::parse();
  friend void MR::App::sort_arguments(const std::vector<std::string> &arguments);
};

//! object storing information about option parsed from command-line
/*! this is the object stored in the App::options vector, and the type
 * returned by App::get_options(). */
class ParsedOption {
public:
  ParsedOption(const Option *option, const std::vector<std::string> &arguments, size_t index);

  //! reference to the corresponding Option entry in the OPTIONS section
  const Option *opt;
  //! list of arguments supplied to the option
  std::vector<std::string> args;
  //! the index of this option in the raw command-line arguments list
  size_t index;

  //! the value supplied for the num-th sub-argument (leaf) of this option
  /*! Tuple arguments are flattened: num indexes the option's leaf sub-arguments in
   * command-line order, so opt[0] is the first token, opt[1] the second, and so on,
   * irrespective of whether those tokens belong to a tuple or to separate arguments. */
  ParsedArgument operator[](size_t num) const;

  //! the value supplied for the sub-argument (leaf) whose id matches the supplied name
  /*! Convenience accessor for reading a tuple's fields by name rather than by position,
   * e.g. opt[0]["bvecs"]. The name must match one of the option's leaf sub-argument ids. */
  ParsedArgument operator[](std::string_view name) const;

  //! check whether this option matches the name supplied
  bool operator==(std::string_view match) const;

  //! flag this parsed option as having been read by the executing command
  /*! Called from every option-reading path (the get_options() family and both operator[]
   * overloads) so that, once run() has returned, any user-specified option that was never
   * read can be reported (see App::check_unused_options()). Marking is intentionally a
   * "was this option consulted at all" signal: merely testing an option's presence counts
   * as reading it. */
  void mark_accessed() const noexcept { accessed = true; }

  //! whether any code path has consulted this option's presence or value
  bool was_accessed() const noexcept { return accessed; }

private:
  //! whether the executing command has consulted this option (see mark_accessed())
  mutable bool accessed{false};
};

//! the list of arguments parsed from the command-line
extern std::vector<ParsedArgument> argument;
//! the list of options parsed from the command-line
extern std::vector<ParsedOption> option;

//! additional description of the command over and above the synopsis
/*! This is designed to be used within each command's usage() function. Add
 * a paragraph to the description using the '+' operator, e.g.:
 * \code
 * void usage() {
 *   DESCRIPTION
 *   + "This command can be used in lots of ways "
 *     "and is very versatile."
 *
 *   + "More description in this paragraph. It has lots of options "
 *     "and arguments.";
 * }
 * \endcode
 */
extern Description DESCRIPTION;

//! example usages of the command
/*! This is designed to be used within each command's usage() function. Add
 * various examples in order to demonstrate the different syntaxes and/or
 * capabilities of the command, e.g.:
 * \code
 * void usage() {
 *   ...
 *
 *   EXAMPLES
 *   + Example ("Perform the command's default functionality",
 *              "input2output input.mif output.mif",
 *              "The default usage of this command is as trivial as "
 *              "providing the name of the command, then the input image, "
 *              "then the output image.");
 * }
 * \endcode
 */
extern ExampleList EXAMPLES;

//! the arguments expected by the command
/*! This is designed to be used within each command's usage() function. Add
 * argument and their description using the Argument class and the'+'
 * operator, e.g.:
 * \code
 * void usage() {
 *   ...
 *
 *   ARGUMENTS
 *   + Argument ("in", "the input image").type_image_in()
 *   + Argument ("factor", "the factor to use in the analysis").type_float()
 *   + Argument ("out", "the output image").type_image_out();
 * }
 * \endcode
 */
extern ArgumentList ARGUMENTS;

//! the options accepted by the command
/*! This is designed to be used within each command's usage() function. Add
 * options, their arguments, and their description using the Option and
 * Argument classes and the'+' operator, e.g.:
 * \code
 * void usage() {
 *   ...
 *
 *   OPTIONS
 *   + Option ("advanced", "use advanced analysis")
 *
 *   + Option ("range", "the range to use in the analysis")
 *   +   Argument ("min").type_float()
 *   +   Argument ("max").type_float();
 * }
 * \endcode
 */
extern OptionList OPTIONS;

//! a set of option identifiers declared mutually exclusive independently of their OptionGroup
/*! Where an OptionGroup constraint (OptionGroup::mutually_exclusive() etc.) cannot express a
 *  mutual exclusion — because the conflicting options belong to different groups, or are only a
 *  subset of a group that also contains unrelated options — the exclusion is declared instead as
 *  a set of option ids in the command-level MUTUALLY_EXCLUSIVE_OPTIONS global. At most one of the
 *  listed options may be specified on the command-line. Options are identified by exact id (no
 *  leading dash), resolved across the entire option hierarchy. */
class MutuallyExclusiveOptions : public std::vector<std::string> {
public:
  MutuallyExclusiveOptions() = default;
  MutuallyExclusiveOptions(std::initializer_list<std::string> ids) : std::vector<std::string>(ids) {}
};

//! the command-declared cross-group mutual-exclusion sets (see MutuallyExclusiveOptions)
/*! Populated within a command's usage() function, e.g.:
 * \code
 *   MUTUALLY_EXCLUSIVE_OPTIONS = {{"min_factor", "min_coeff"}, {"max_factor", "max_coeff"}};
 * \endcode
 * Each set is enforced at parse time; a violation names the offending options. */
extern std::vector<MutuallyExclusiveOptions> MUTUALLY_EXCLUSIVE_OPTIONS;

//! the ordered sub-interfaces of a hierarchical command
/*! Populated within a command's usage() function to make it hierarchical: the
 * sub-interface is selected by the first positional command-line token. A command
 * that declares SUBCOMMANDS must declare no top-level ARGUMENTS; any options declared
 * in OPTIONS are treated as common to (and available within) every sub-interface. */
extern SubcommandList SUBCOMMANDS;

//! the displayed name of a hierarchical command's selection positional
/*! Defaults to "algorithm"; a command may set this (e.g. "operation", "filter") to
 * describe the nature of its sub-interface selection in help and export output. */
extern std::string SUBCOMMANDS_SELECTOR;

//! the id of the sub-interface selected on the command-line, for a hierarchical command
/*! Valid within run(); empty for non-hierarchical commands. A hierarchical command's
 * run() reads this to dispatch to the selected sub-interface's implementation. */
std::string get_subcommand();

//! set to false if command can operate with no arguments
/*! By default, the help page is shown command is invoked without
 * arguments. Some commands (e.g. MRView) can operate without arguments. */
extern bool REQUIRES_AT_LEAST_ONE_ARGUMENT;

//! set the author of the command
extern std::string AUTHOR;

//! set the copyright notice if different from that used in MRtrix
extern std::string COPYRIGHT;

//! set a one-sentence synopsis for the command
extern std::string SYNOPSIS;

//! add references to command help page
/*! Like the description, use the '+' operator to add paragraphs (typically
 * one citation per paragraph)." */
extern Description REFERENCES;

//! the group of standard options for all commands
extern const OptionGroup _standard_options;

//! return all command-line options matching \c name
/*! This returns a vector of vectors, where each top-level entry
 * corresponds to a distinct instance of the option, and each entry within
 * a top-level entry corresponds to a argument supplied to that option.
 *
 * Individual options can be retrieved easily using the as_* methods, or
 * implicit type-casting.  Any relevant range checks are performed at this
 * point, based on the original App::Option specification. For example:
 * \code
 * Options opt = get_options ("myopt");
 * if (opt.size()) {
 *    std::string arg1 = opt[0][0];
 *    int arg2 = opt[0][1];
 *    float arg3 = opt[0][2];
 *    std::vector<int> arg4 = opt[0][3];
 *    auto values = opt[0][4].as_sequence_float();
 * }
 * \endcode */
std::vector<ParsedOption> get_options(std::string_view name);

//! Returns the option value if set, and the default otherwise.
/*! Only be used for command-line options that do not specify
 * .allow_multiple(), and that have only one associated Argument.
 *
 * Use:
 * \code
 *  float arg1 = get_option_value("myopt", arg1_default);
 *  int arg2 = get_option_value("myotheropt", arg2_default);
 * \endcode
 */
template <typename T> inline T get_option_value(std::string_view name, const T default_value) {
  auto opt = get_options(name);
  switch (opt.size()) {
  case 0:
    return default_value;
  case 1:
    if (opt[0].opt->arity() == 1)
      return opt[0][0];
  default:
    assert(false);
    throw Exception("Internal error parsing command-line option \"-" + name + "\"");
  }
}

//! Returns the enum choice selected for an option, and the default otherwise.
/*! Only be used for command-line options that do not specify
 * .allow_multiple(), and that have only one associated Argument declared
 * using Argument::type_choice<Enum>().
 */
template <typename Enum> inline Enum get_option_choice(std::string_view name, const Enum default_value) {
  static_assert(std::is_enum_v<Enum>, "Template parameter must be an enum type");

  auto opt = get_options(name);
  switch (opt.size()) {
  case 0:
    return default_value;
  case 1:
    if (opt[0].opt->arity() == 1)
      return MR::Enum::from_name<Enum>(std::string_view(opt[0][0]));
  default:
    assert(false);
    throw Exception("Internal error parsing command-line option \"-" + name + "\"");
  }
}

//! Returns the user-specified choice in a std::optional<> if present, std::nullopt otherwise.
template <typename T>
typename std::enable_if<!std::is_enum_v<T>, std::optional<T>>::type get_optional(std::string_view name) {
  auto opt = get_options(name);
  switch (opt.size()) {
  case 0:
    return std::nullopt;
  case 1:
    return static_cast<T>(opt[0][0]);
  default:
    assert(false);
    throw Exception("Internal error parsing command-line option \"-" + name + "\"");
  }
}
template <typename Enum>
typename std::enable_if<std::is_enum_v<Enum>, std::optional<Enum>>::type get_optional(std::string_view name) {
  auto opt = get_options(name);
  switch (opt.size()) {
  case 0:
    return std::nullopt;
  case 1:
    return MR::Enum::from_name<Enum>(std::string_view(opt[0][0]));
  default:
    assert(false);
    throw Exception("Internal error parsing command-line option \"-" + name + "\"");
  }
}

//! warn about any user-specified option that the command never consulted [used internally]
/*! Called from the command driver once run() has returned. Every option-reading path
 * (the get_options() family and ParsedOption::operator[]) flags the option it reads as
 * accessed; any explicitly-specified option that remains unaccessed is reported, since it
 * had no bearing on execution (for example, an option that is not applicable to the
 * requested mode of operation). Standard options are exempt, as the framework consumes
 * them uniformly and lazily. */
void check_unused_options();

//! flag every parsed instance of the given option as having been consulted
/*! For the rare commands that consume options outside the get_options() family — e.g. mrcalc,
 * which identifies operators via match_option() and processes them through its own reverse-Polish
 * token loop rather than the standard accessors — so that check_unused_options() does not
 * misreport those options as unused. The command calls this for each option it actually
 * consumes. */
void mark_option_accessed(const Option *opt);

//! convenience function provided mostly to ease writing Exception strings
std::string operator+(const char *const left, const App::ParsedArgument &right); // check_syntax off

std::ostream &operator<<(std::ostream &stream, const App::ParsedArgument &arg);

} // namespace MR::App

//! @}
