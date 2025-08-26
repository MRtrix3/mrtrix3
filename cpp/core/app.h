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

#include <cstring>
#include <limits>
#include <string>
#include <thread>

#ifdef None
#undef None
#endif

#include "cmdline_option.h"
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
extern void (*check_overwrite_files_func)(const std::string &name);
extern bool fail_on_warn;
extern bool terminal_use_colour;
extern const std::thread::id main_thread_ID;

extern std::vector<std::string> raw_arguments_list;

extern const char *project_version;
extern const char *project_build_date;

std::string help_head(int format);
std::string help_synopsis(int format);
std::string help_tail(int format);
std::string usage_syntax(int format);

//! \addtogroup CmdParse
// @{

//! vector of strings to hold more comprehensive command description
class Description : public std::vector<const char *> {
public:
  Description &operator+(const char *text);

  Description &operator+(const char *const text[]);

  std::string syntax(int format) const;
};

//! object for storing a single example command usage
class Example {
public:
  Example(const std::string &title, const std::string &code, const std::string &description);
  const std::string title, code, description;

  operator std::string() const;
  std::string syntax(int format) const;
};

//! a class to hold the list of Example's
class ExampleList : public std::vector<Example> {
public:
  ExampleList &operator+(const Example &example);

  std::string syntax(int format) const;
};

//! a class to hold the list of Argument's
class ArgumentList : public std::vector<Argument> {
public:
  ArgumentList &operator+(const Argument &argument);

  std::string syntax(int format) const;
};

//! a class to hold the list of option groups
class OptionList : public std::vector<OptionGroup> {
public:
  OptionList &operator+(const OptionGroup &option_group);

  OptionList &operator+(const Option &option);

  OptionList &operator+(const Argument &argument);

  OptionGroup &back();

  std::string syntax(int format) const;
};

void check_overwrite(const std::string &name);

//! initialise MRtrix and parse command-line arguments
/*! this function must be called from within main(), immediately after the
 * argument and options have been specified, and before any further
 * processing takes place. */
void init(int argc, const char *const *argv);

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
  operator std::string() const { return p; }

  const std::string &as_text() const {
    assert(arg->types | Text);
    return p;
  }
  bool as_bool() const {
    assert(arg->types | Boolean);
    return to<bool>(p);
  }
  int64_t as_int() const;
  uint64_t as_uint() const;
  default_type as_float() const;

  std::vector<int32_t> as_sequence_int() const;

  std::vector<uint32_t> as_sequence_uint() const;

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
  operator std::vector<int32_t>() const { return as_sequence_int(); }
  operator std::vector<uint32_t>() const { return as_sequence_uint(); }
  operator std::vector<default_type>() const { return as_sequence_float(); }

  const char *c_str() const { return p.c_str(); }

  //! the index of this argument in the raw command-line arguments list
  size_t index() const { return index_; }

private:
  const Option *opt;
  const Argument *arg;
  std::string p;
  size_t index_;

  ParsedArgument(const Option *option, const Argument *argument, std::string text, size_t index);

  void error(Exception &e) const;

  friend class ParsedOption;
  friend class Options;
  friend void MR::App::init(int argc, const char *const *argv);
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

  ParsedArgument operator[](size_t num) const;

  //! check whether this option matches the name supplied
  bool operator==(const char *match) const;
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
extern OptionGroup __standard_options;

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
const std::vector<ParsedOption> get_options(const std::string &name);

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
template <typename T> inline T get_option_value(const std::string &name, const T default_value) {
  auto opt = get_options(name);
  switch (opt.size()) {
  case 0:
    return default_value;
  case 1:
    if (opt[0].opt->size() == 1)
      return opt[0][0];
  default:
    assert(false);
    throw Exception("Internal error parsing command-line option \"-" + name + "\"");
  }
}

//! convenience function provided mostly to ease writing Exception strings
std::string operator+(const char *left, const App::ParsedArgument &right);

std::ostream &operator<<(std::ostream &stream, const App::ParsedArgument &arg);

} // namespace MR::App

//! @}
