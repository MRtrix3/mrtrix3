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

#include <algorithm>
#include <cerrno>
#include <clocale>
#include <cstddef>
#include <fcntl.h>
#include <filesystem>
#include <locale>
#include <unistd.h>

#include "app.h"
#include "cmdline_option.h"
#include "debug.h"
#include "env.h"
#include "exception.h"
#include "executable_version.h"
#include "file/config.h"
#include "file/path.h"
#include "mrtrix.h"
#include "mrtrix_version.h"
#include "progressbar.h"
#include "signal_handler.h"

namespace MR::App {

Description DESCRIPTION;
ExampleList EXAMPLES;
ArgumentList ARGUMENTS;
OptionList OPTIONS;
SubcommandList SUBCOMMANDS;
std::string SUBCOMMANDS_SELECTOR = "algorithm";
std::string SUBCOMMAND_SELECTED_ID;
Description REFERENCES;
bool REQUIRES_AT_LEAST_ONE_ARGUMENT = true;

std::string get_subcommand() { return SUBCOMMAND_SELECTED_ID; }

const std::string help_command = "less -X";

const HelpFormatting help_formatting{
    80,      // const ssize_t width
    {0, 4},  // const Indents purpose_indents
    {8, 20}, // const Indents arg_indents
    {2, 20}, // const Indents option_indents
    7        // const ssize_t example_indent
};

const std::string core_reference =
    "Tournier, J.-D.; Smith, R. E.; Raffelt, D.; Tabbara, R.; Dhollander, T.; Pietsch, M.; Christiaens, D.; " //
    "Jeurissen, B.; Yeh, C.-H. & Connelly, A. "                                                               //
    "MRtrix3: A fast, flexible and open software framework for medical image processing and visualisation. "  //
    "NeuroImage, 2019, 202, 116137";                                                                          //

// clang-format off
const OptionGroup _standard_options = OptionGroup("Standard options")
  + Option("info", "display information messages.")
  + Option("quiet",
           "do not display information messages or progress status; "
           "alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable"
           " to a non-empty string.")
  + Option("debug", "display debugging messages & debug input data.")
  + Option("force",
           "force overwrite of output files"
           " (caution: using the same file as input and output might cause unexpected behaviour).")
  + Option("nthreads",
           "use this number of threads in multi-threaded applications"
           " (set to 0 to disable multi-threading).")
    + Argument("number").type_integer(0)
  + Option("config", "temporarily set the value of an MRtrix config file entry.").allow_multiple()
    + Argument("key_value").type_tuple({Argument("key").type_text(), Argument("value").type_text()})
  + Option("help", "display this information page and exit.")
  + Option("version", "display version information and exit.");
// clang-format on

std::string AUTHOR{};
std::string COPYRIGHT = "Copyright (c) 2008-2026 the MRtrix3 contributors.\n"
                        "\n"
                        "This Source Code Form is subject to the terms of the Mozilla Public\n"
                        "License, v. 2.0. If a copy of the MPL was not distributed with this\n"
                        "file, You can obtain one at http://mozilla.org/MPL/2.0/.\n"
                        "\n"
                        "Covered Software is provided under this License on an \"as is\"\n"
                        "basis, without warranty of any kind, either expressed, implied, or\n"
                        "statutory, including, without limitation, warranties that the\n"
                        "Covered Software is free of defects, merchantable, fit for a\n"
                        "particular purpose or non-infringing.\n"
                        "See the Mozilla Public License v. 2.0 for more details.\n"
                        "\n"
                        "For more details, see http://www.mrtrix.org/.\n";
std::string SYNOPSIS{};

std::string NAME;
std::string command_history_string;
std::vector<ParsedArgument> argument;
std::vector<ParsedOption> option;

// ENVVAR name: MRTRIX_QUIET
// ENVVAR Do not display information messages or progress status. This has
// ENVVAR the same effect as the ``-quiet`` command-line option. If set,
// ENVVAR supersedes the MRTRIX_LOGLEVEL environment variable.

// ENVVAR name: MRTRIX_LOGLEVEL
// ENVVAR Set the default terminal verbosity. Default terminal verbosity
// ENVVAR is 1. This has the same effect as the ``-quiet`` (0),
// ENVVAR ``-info`` (2) or ``-debug`` (3) comand-line options.
int log_level = MR::get_env("MRTRIX_QUIET").has_value() ? 0 : MR::get_env("MRTRIX_LOGLEVEL", 1);

int exit_error_code = 0;
bool fail_on_warn = false;
bool terminal_use_colour = true;
const std::thread::id main_thread_ID = std::this_thread::get_id();

const std::string project_version;
const std::string project_build_date;

std::vector<std::string> raw_arguments_list;

bool overwrite_files = false;
void (*check_overwrite_files_func)(const std::filesystem::path &name) = nullptr;

namespace {

inline void get_matches(std::vector<const Option *> &candidates, const OptionGroup &group, std::string_view stub) {
  for (size_t i = 0; i < group.size(); ++i) {
    if (stub.compare(0, stub.size(), std::string(group[i].id), 0, stub.size()) == 0)
      candidates.push_back(&group[i]);
  }
}

inline std::string::size_type characters_ignoring_emphasis(std::string_view text) {
  return text.size() - 2 * std::count(text.begin(), text.end(), 0x08U);
}

inline void resize(std::string &text, size_t new_size, char fill) {
  text.resize(text.size() + new_size - characters_ignoring_emphasis(text), fill);
}

std::string paragraph(std::string_view header, std::string_view text, const HelpFormatting::Indents indents) {
  std::string out;
  std::string line = std::string(indents.header, ' ') + std::string(header) + " ";
  if (characters_ignoring_emphasis(line) < indents.main)
    resize(line, indents.main, ' ');

  std::vector<std::string> paragraphs = split(text, "\n");

  for (size_t n = 0; n < paragraphs.size(); ++n) {
    size_t i = 0;
    std::vector<std::string> words = split(paragraphs[n]);
    while (i < words.size()) {
      do {
        line += " " + words[i++];
        if (i >= words.size())
          break;
      } while (characters_ignoring_emphasis(line) + 1 + characters_ignoring_emphasis(words[i]) < help_formatting.width);
      out += line + "\n";
      line = std::string(indents.main, ' ');
    }
  }
  return out;
}

std::string bold(std::string_view text) {
  std::string retval(3 * text.size(), '\0');
  for (size_t n = 0; n < text.size(); ++n) {
    retval[3 * n] = retval[3 * n + 2] = text[n];
    retval[3 * n + 1] = 0x08U;
  }
  return retval;
}

std::string underline(std::string_view text, bool ignore_whitespace = false) {
  size_t m(0);
  std::string retval(3 * text.size(), '\0');
  for (size_t n = 0; n < text.size(); ++n) {
    if (ignore_whitespace and text[n] == ' ')
      retval[m++] = ' ';
    else
      retval[m++] = '_';
    retval[m++] = 0x08U;
    retval[m++] = text[n];
  }
  return retval;
}

} // namespace

// ---------------------------------------------------------------------------------------
// Hierarchical-command (subparser-equivalent) machinery.
//   A hierarchical command declares an ordered SUBCOMMANDS list; the sub-interface is
//   selected by the first positional command-line token. The selected sub-interface is
//   "installed" over the usage() globals (merged with the command's common options and
//   the standard options), so that the ordinary parser, help renderer and export
//   renderers operate on it unchanged. The command's common interface is snapshotted
//   first so it can be re-installed for each sub-interface (e.g. when rendering the
//   concatenated top-level export).
// ---------------------------------------------------------------------------------------

//! the machine-readable / human export format requested for a (sub-)interface
enum class ExportFormat { FullUsage, Markdown, Rst };

namespace {

//! snapshot of a hierarchical command's common (top-level) interface
struct TopLevelInterface {
  bool captured{false};
  OptionList common_options;
  Description references;
  std::string author;
  std::string copyright;
};
TopLevelInterface top_level_interface;

//! indefinite article appropriate to the selection-positional noun
std::string selector_article() {
  if (!SUBCOMMANDS_SELECTOR.empty()) {
    switch (std::tolower(static_cast<unsigned char>(SUBCOMMANDS_SELECTOR.front()))) {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
      return "an";
    default:
      break;
    }
  }
  return "a";
}

//! the fixed selection help string presented for the selection positional
std::string selection_help_string() {
  return "Select the " + SUBCOMMANDS_SELECTOR + " to be used;" +                         //
         " additional details and options become available once " + selector_article() + //
         " " + SUBCOMMANDS_SELECTOR + " is nominated. Options are: " +                   //
         join(SUBCOMMANDS.ids(), ", ");                                                  //
}

//! snapshot the common interface before any sub-interface is installed over the globals
void capture_top_level_interface() {
  if (top_level_interface.captured)
    return;
  top_level_interface.common_options = OPTIONS;
  top_level_interface.references = REFERENCES;
  top_level_interface.author = AUTHOR;
  top_level_interface.copyright = COPYRIGHT;
  top_level_interface.captured = true;
}

//! install a sub-interface over the usage() globals, merged with common + standard options
void install_subcommand(const Subcommand &sub) {
  capture_top_level_interface();
  SYNOPSIS = sub.synopsis;
  AUTHOR = sub.author.empty() ? top_level_interface.author : sub.author;
  COPYRIGHT = sub.copyright.empty() ? top_level_interface.copyright : sub.copyright;
  DESCRIPTION = sub.description;
  EXAMPLES = sub.examples;
  ARGUMENTS = sub.arguments;
  OPTIONS = OptionList();
  for (const auto &group : sub.options)
    OPTIONS.push_back(group);
  for (const auto &group : top_level_interface.common_options)
    OPTIONS.push_back(group);
  REFERENCES = top_level_interface.references;
  for (const auto &reference : sub.references)
    REFERENCES.push_back(reference);
}

const Option *standard_option(const char *id) { // check_syntax off
  for (const auto &opt : _standard_options)
    if (opt.is(id))
      return &opt;
  return nullptr;
}

enum class HelpVersion { None, Help, Version };

//! detect a -help / -version request among tokens, resolved against the current option set
/*! Per-token matching mirrors the Python parser: unknown-option matches are ignored so
 *  that -help / -version take precedence over otherwise-invalid content. */
HelpVersion prescan_help_version(const std::vector<std::string> &tokens) {
  const Option *const help_opt = standard_option("help");
  const Option *const version_opt = standard_option("version");
  for (const auto &token : tokens) {
    const Option *opt = nullptr;
    try {
      opt = match_option(token);
    } catch (Exception &) {
      continue;
    }
    if (opt != nullptr && opt == help_opt)
      return HelpVersion::Help;
    if (opt != nullptr && opt == version_opt)
      return HelpVersion::Version;
  }
  return HelpVersion::None;
}

} // namespace

// forward declarations (mutual references between the exporters and per-sub rendering)
std::string full_usage();
std::string markdown_usage();
std::string restructured_text_usage();
std::string get_help_string(const bool format);
namespace {
std::string render_subcommand(const Subcommand &sub, ExportFormat format);
std::string subcommand_rst_section(const Subcommand &sub);
} // namespace

std::string help_head(const bool format) {
  if (!format) {
    return std::string(NAME) + ": " +
           (project_version.empty() ? ("part of the MRtrix3 package, version " + mrtrix_version)
                                    : "external MRtrix3 project, version " + project_version +
                                          "\nbuilt against MRtrix3 version " + mrtrix_version) +
           "\n\n";
  }

  const std::string version_string =
      project_version.empty() ? (std::string("MRtrix ") + mrtrix_version) : ("Version " + project_version);

  const std::string date(project_version.empty() ? build_date : project_build_date);

  auto safe_padding = [](std::ptrdiff_t want, std::size_t minimum = 1) -> std::size_t {
    if (want < static_cast<std::ptrdiff_t>(minimum))
      return minimum;
    return static_cast<std::size_t>(want);
  };

  // compute requested padding to position the program name
  const std::ptrdiff_t requested_padding = 40 -
                                           static_cast<std::ptrdiff_t>(characters_ignoring_emphasis(version_string)) -
                                           (static_cast<std::ptrdiff_t>(characters_ignoring_emphasis(App::NAME)) / 2);
  std::string topline = version_string + std::string(safe_padding(requested_padding), ' ') + bold(App::NAME);

  // compute requested padding to right-align the date
  const std::ptrdiff_t requested_padding2 = 80 - static_cast<std::ptrdiff_t>(characters_ignoring_emphasis(topline)) -
                                            static_cast<std::ptrdiff_t>(characters_ignoring_emphasis(date));
  topline += std::string(safe_padding(requested_padding2), ' ') + date;

  if (!project_version.empty())
    topline += std::string("\nusing MRtrix3 ") + mrtrix_version;

  return topline + "\n\n     " + bold(NAME) + ": " +
         (project_version.empty() ? "part of the MRtrix3 package" : "external MRtrix3 project") + "\n\n";
}

std::string help_synopsis(const bool format) {
  if (!format)
    return SYNOPSIS;
  return bold("SYNOPSIS") + "\n\n" + paragraph("", SYNOPSIS, help_formatting.purpose_indents) + "\n";
}

std::string help_tail(const bool format) {
  std::string retval;
  if (!format)
    return retval;

  return bold("AUTHOR") + "\n" + paragraph("", AUTHOR, help_formatting.purpose_indents) + "\n" + bold("COPYRIGHT") +
         "\n" + paragraph("", COPYRIGHT, help_formatting.purpose_indents) + "\n" + [&]() {
           std::string s = bold("REFERENCES") + "\n";
           for (size_t n = 0; n < REFERENCES.size(); ++n)
             s += paragraph("", REFERENCES[n], help_formatting.purpose_indents) + "\n";
           s += paragraph("", core_reference, help_formatting.purpose_indents) + "\n";
           return s;
         }();
}

std::string usage_syntax(const bool format) {
  std::string s = "USAGE";
  if (format)
    s = bold(s) + "\n\n     ";
  else
    s += ": ";
  s += (format ? underline(NAME, true) : NAME);

  // A hierarchical command presents the selection positional first, with a trailing
  //   ellipsis standing in for the selected sub-interface's arguments and options.
  if (!SUBCOMMANDS.empty())
    return s + " " + SUBCOMMANDS_SELECTOR + " [ options ] ...\n\n";

  s += " [ options ]";

  for (size_t i = 0; i < ARGUMENTS.size(); ++i) {

    if (ARGUMENTS[i].flags.optional())
      s += " [";
    s += std::string(" ") + ARGUMENTS[i].syntax_id();

    if (ARGUMENTS[i].flags.allow_multiple()) {
      if (ARGUMENTS[i].flags.required())
        s += std::string(" [ ") + ARGUMENTS[i].syntax_id();
      s += " ...";
    }
    if (ARGUMENTS[i].flags.any())
      s += " ]";
  }
  return s + "\n\n";
}

Description &Description::operator+(const char *text) { // check_syntax off
  emplace_back(std::string(text));
  return *this;
}

Description &Description::operator+(std::string_view text) {
  emplace_back(std::string(text));
  return *this;
}

Description &Description::operator+(const char *const text[]) { // check_syntax off
  for (const char *const *p = text; *p != nullptr; ++p)         // check_syntax off
    emplace_back(std::string(*p));
  return *this;
}

std::string Description::syntax(const bool format) const {
  if (!size())
    return std::string();
  std::string s;
  if (format)
    s += bold("DESCRIPTION") + "\n\n";
  for (size_t i = 0; i < size(); ++i)
    s += paragraph("", (*this)[i], help_formatting.purpose_indents) + "\n";
  return s;
}

Example::Example(std::string_view title, std::string_view code, std::string_view description)
    : title(title), code(code), description(description) {}

Example::operator std::string() const { return title + ": $ " + code + "  " + description; }

std::string Example::syntax(const bool format) const {
  std::string s = paragraph("",                                 //
                            format                              //
                                ? underline(title + ":") + "\n" //
                                : title + ": ",                 //
                            help_formatting.purpose_indents);   //
  s += std::string(help_formatting.example_indent, ' ') + "$ " + code + "\n";
  if (!description.empty())
    s += paragraph("", description, help_formatting.purpose_indents);
  if (format)
    s += "\n";
  return s;
}

ExampleList &ExampleList::operator+(const Example &example) {
  push_back(example);
  return *this;
}

std::string ExampleList::syntax(const bool format) const {
  if (!size())
    return std::string();
  std::string s;
  if (format)
    s += bold("EXAMPLE USAGES") + "\n\n";
  for (size_t i = 0; i < size(); ++i)
    s += (*this)[i].syntax(format);
  return s;
}

std::string Argument::syntax_id() const {
  if (elements.empty())
    return id;
  std::string result;
  for (size_t i = 0; i < elements.size(); ++i) {
    if (i != 0)
      result += " ";
    result += elements[i].id;
  }
  return result;
}

std::string Argument::syntax(const bool format) const {
  const std::string header = syntax_id();
  std::string retval = paragraph((format ? underline(header, true) : header), desc, help_formatting.arg_indents);
  // For a tuple argument, list each sub-argument (id and description) beneath the summary line.
  const HelpFormatting::Indents element_indents{help_formatting.arg_indents.header + 2,
                                                help_formatting.arg_indents.main + 2};
  for (const auto &element : elements) {
    if (element.desc.empty())
      continue;
    retval += paragraph((format ? underline(element.id, true) : element.id), element.desc, element_indents);
  }
  if (format)
    retval += "\n";
  return retval;
}

ArgumentList &ArgumentList::operator+(const Argument &argument) {
  push_back(argument);
  return *this;
}

std::string ArgumentList::syntax(const bool format) const {
  std::string s;
  for (size_t i = 0; i < size(); ++i)
    s += (*this)[i].syntax(format);
  return s + "\n";
}

std::string Option::syntax(const bool format) const {
  std::string opt("-");
  opt += id;

  if (format)
    opt = underline(opt);

  for (const Argument *leaf : leaves())
    opt += std::string(" ") + leaf->id;

  if (format && flags.allow_multiple())
    opt += "  (multiple uses permitted)";

  if (format)
    opt = "  " + opt + "\n" + paragraph("", desc, help_formatting.purpose_indents) + "\n";
  else
    opt = paragraph(opt, desc, help_formatting.option_indents);

  // For a tuple argument, list each described sub-argument (id and description) beneath the option.
  for (const auto &arg : *this) {
    if (!arg.is_tuple())
      continue;
    for (const auto &element : arg.elements) {
      if (element.desc.empty())
        continue;
      opt += paragraph((format ? underline(element.id, true) : element.id), element.desc, help_formatting.arg_indents);
    }
  }
  return opt;
}

std::string OptionGroup::header(const bool format) const {
  return format ? bold(name) + "\n\n" : std::string(name) + ":\n";
}

std::string OptionGroup::contents(const bool format) const {
  std::string s;
  for (size_t i = 0; i < size(); ++i)
    s += (*this)[i].syntax(format);
  return s;
}

std::string OptionGroup::footer(const bool format) { return format ? "" : "\n"; }

OptionList &OptionList::operator+(const OptionGroup &option_group) {
  push_back(option_group);
  return *this;
}

OptionGroup &OptionList::back() {
  if (empty())
    push_back(OptionGroup());
  return std::vector<OptionGroup>::back();
}

OptionList &OptionList::operator+(const Option &option) {
  back() + option;
  return *this;
}

OptionList &OptionList::operator+(const Argument &argument) {
  back() + argument;
  return *this;
}

std::string OptionList::syntax(const bool format) const {
  std::vector<std::string> group_names;
  for (size_t i = 0; i < size(); ++i) {
    if (std::find(group_names.begin(), group_names.end(), (*this)[i].name) == group_names.end())
      group_names.push_back((*this)[i].name);
  }

  std::string s;
  for (size_t i = 0; i < group_names.size(); ++i) {
    size_t n = i;
    while ((*this)[n].name != group_names[i])
      ++n;
    s += (*this)[n].header(format);
    while (n < size()) {
      if ((*this)[n].name == group_names[i])
        s += (*this)[n].contents(format);
      ++n;
    }
    s += OptionGroup::footer(format);
  }

  return s;
}

std::string Argument::usage() const {
  // A tuple argument is serialised as one ARGUMENT line per sub-argument, each carrying the
  //   tuple's optional / allow_multiple flags so that repeated groups remain identifiable.
  if (is_tuple()) {
    std::string s;
    for (const auto &element : elements) {
      Argument line(element);
      line.flags = flags;
      s += line.usage();
    }
    return s;
  }

  std::ostringstream stream;
  stream << "ARGUMENT " << id                            //
         << " " << (flags.optional() ? '1' : '0')        //
         << " " << (flags.allow_multiple() ? '1' : '0'); //

  if (types[ArgTypeFlags::Text])
    stream << " TEXT";
  if (types[ArgTypeFlags::Boolean])
    stream << " BOOL";
  if (types[ArgTypeFlags::Integer])
    stream << " INT " << int_limits.min() << " " << int_limits.max();
  if (types[ArgTypeFlags::Float])
    stream << " FLOAT " << float_limits.min() << " " << float_limits.max();
  if (types[ArgTypeFlags::FileIn])
    stream << " FILEIN";
  if (types[ArgTypeFlags::FileOut])
    stream << " FILEOUT";
  if (types[ArgTypeFlags::DirectoryIn])
    stream << " DIRIN";
  if (types[ArgTypeFlags::DirectoryOut])
    stream << " DIROUT";
  if (types[ArgTypeFlags::ImageIn])
    stream << " IMAGEIN";
  if (types[ArgTypeFlags::ImageOut])
    stream << " IMAGEOUT";
  if (types[ArgTypeFlags::IntSeq])
    stream << " ISEQ";
  if (types[ArgTypeFlags::FloatSeq])
    stream << " FSEQ";
  if (types[ArgTypeFlags::TracksIn])
    stream << " TRACKSIN";
  if (types[ArgTypeFlags::TracksOut])
    stream << " TRACKSOUT";
  if (types[ArgTypeFlags::Choice]) {
    stream << " CHOICE";
    for (const auto &p : choices)
      stream << " " << p;
  }
  stream << "\n";
  if (!desc.empty())
    stream << desc << "\n";

  return stream.str();
}

std::string Option::usage() const {
  std::ostringstream stream;
  stream << "OPTION " << id << " " << (flags.optional() ? '1' : '0') << " " << (flags.allow_multiple() ? '1' : '0')
         << "\n";

  if (!desc.empty())
    stream << desc << "\n";

  for (size_t i = 0; i < size(); ++i)
    stream << (*this)[i].usage();

  return stream.str();
}

std::string get_help_string(const bool format) {
  // For a hierarchical command the sole leading positional is the sub-interface selection,
  //   rendered from the fixed selection help string in place of any real ARGUMENTS.
  std::string arguments_section;
  if (SUBCOMMANDS.empty()) {
    arguments_section = ARGUMENTS.syntax(format);
  } else {
    ArgumentList selection;
    selection.push_back(Argument(SUBCOMMANDS_SELECTOR, selection_help_string()));
    arguments_section = selection.syntax(format);
  }
  return help_head(format) + help_synopsis(format) + usage_syntax(format) + arguments_section +
         DESCRIPTION.syntax(format) + EXAMPLES.syntax(format) + OPTIONS.syntax(format) +
         _standard_options.header(format) + _standard_options.contents(format) + MR::App::OptionGroup::footer(format) +
         help_tail(format);
}

void print_help() {
  File::Config::init();

  // CONF option: HelpCommand
  // CONF default: less -X
  // CONF The command to use to display each command's help page (leave
  // CONF empty to send directly to the terminal).
  const std::string help_display_command = File::Config::get("HelpCommand", help_command);

  if (!help_display_command.empty()) {
    std::string help_string = get_help_string(1);
    FILE *file = popen(help_display_command.c_str(), "w");
    if (!file) {
      INFO("error launching help display command \"" + help_display_command + "\": " + MR::C_strerror(errno));
    } else if (fwrite(help_string.c_str(), 1, help_string.size(), file) != help_string.size()) {
      INFO("error sending help page to display command \"" + help_display_command + "\": " + MR::C_strerror(errno));
    }

    if (pclose(file) == 0)
      return;

    INFO("error launching help display command \"" + help_display_command + "\"");
  }

  if (!help_display_command.empty())
    INFO("displaying help page using fail-safe output:\n");

  print(get_help_string(0));
}

#ifndef MRTRIX_BUILD_TYPE
#error "MRtrix build type is not defined; you need to re-run configure script"
#endif

std::string version_string() {
  std::string version = "== " + App::NAME + " " + (project_version.empty() ? mrtrix_version : project_version) +
                        " ==\n" + str(8 * sizeof(size_t)) + " bit " + MRTRIX_BUILD_TYPE + ", built " + build_date +
                        (project_version.empty() ? std::string("") : " against MRtrix " + mrtrix_version) +
                        ", using Eigen " + str(EIGEN_WORLD_VERSION) + "." + str(EIGEN_MAJOR_VERSION) + "." +
                        str(EIGEN_MINOR_VERSION) +
                        "\n"
                        "Author(s): " +
                        AUTHOR + "\n" + COPYRIGHT + "\n";

  return version;
}

std::string full_usage() {
  std::string s;
  s += SYNOPSIS + std::string("\n");

  for (size_t i = 0; i < DESCRIPTION.size(); ++i)
    s += DESCRIPTION[i] + std::string("\n");

  for (size_t i = 0; i < EXAMPLES.size(); ++i)
    s += std::string(EXAMPLES[i]) + std::string("\n");

  // For a hierarchical command the sole positional is the sub-interface selection,
  //   emitted as a CHOICE argument enumerating the sub-interface ids (no following help
  //   line, no per-sub-interface recursion at top level).
  if (SUBCOMMANDS.empty()) {
    for (size_t i = 0; i < ARGUMENTS.size(); ++i)
      s += ARGUMENTS[i].usage();
  } else {
    s += "ARGUMENT " + SUBCOMMANDS_SELECTOR + " 0 0 CHOICE " + join(SUBCOMMANDS.ids(), " ") + "\n";
  }

  for (size_t i = 0; i < OPTIONS.size(); ++i)
    for (size_t j = 0; j < OPTIONS[i].size(); ++j)
      s += OPTIONS[i][j].usage();

  for (size_t i = 0; i < _standard_options.size(); ++i)
    s += _standard_options[i].usage();

  return s;
}

std::string markdown_usage() {
  /*
    help_head (format)
    + help_synopsis (format)
    + usage_syntax (format)
    + ARGUMENTS.syntax (format)
    + DESCRIPTION.syntax (format)
    + EXAMPLES.syntax (format)
    + OPTIONS.syntax (format)
    + _standard_options.header (format)
    + _standard_options.contents (format)
    + _standard_options.footer (format)
    + help_tail (format);
  */
  const bool hierarchical = !SUBCOMMANDS.empty();
  std::string s = std::string("## Synopsis\n\n") + SYNOPSIS + "\n\n";

  if (hierarchical) {
    // A hierarchical command presents the sub-interface selection in place of any
    //   positional arguments (of which the top-level command has none).
    s += "## Usage\n\n    " + std::string(NAME) + " " + SUBCOMMANDS_SELECTOR + " [ options ] ...\n\n";
    s += std::string("-  *") + SUBCOMMANDS_SELECTOR + "*: " + selection_help_string() + "\n";
  } else {
    s += "## Usage\n\n    " + std::string(NAME) + " [ options ] ";

    // Syntax line:
    for (size_t i = 0; i < ARGUMENTS.size(); ++i) {

      if (ARGUMENTS[i].flags.optional())
        s += "[";
      s += std::string(" ") + ARGUMENTS[i].syntax_id();

      if (ARGUMENTS[i].flags.allow_multiple()) {
        if (ARGUMENTS[i].flags.required())
          s += std::string(" [ ") + ARGUMENTS[i].syntax_id();
        s += " ...";
      }
      if (ARGUMENTS[i].flags.any())
        s += " ]";
    }
    s += "\n\n";

    // Argument description (tuple sub-arguments listed as an indented sub-list):
    for (size_t i = 0; i < ARGUMENTS.size(); ++i) {
      s += std::string("- *") + ARGUMENTS[i].syntax_id() + "*: " + ARGUMENTS[i].desc + "\n";
      for (const auto &element : ARGUMENTS[i].elements)
        if (!element.desc.empty())
          s += std::string("    - *") + element.id + "*: " + element.desc + "\n";
    }
  }

  if (!DESCRIPTION.empty()) {
    s += "\n## Description\n\n";
    for (size_t i = 0; i < DESCRIPTION.size(); ++i)
      s += std::string(DESCRIPTION[i]) + "\n\n";
  }

  if (!EXAMPLES.empty()) {
    s += "\n## Example usages\n\n";
    for (size_t i = 0; i < EXAMPLES.size(); ++i) {
      s += std::string("__") + EXAMPLES[i].title + ":__\n";
      s += std::string("`$ ") + EXAMPLES[i].code + "`\n";
      if (!EXAMPLES[i].description.empty())
        s += EXAMPLES[i].description + "\n";
      s += "\n";
    }
  }

  std::vector<std::string> group_names;
  for (size_t i = 0; i < OPTIONS.size(); ++i) {
    if (std::find(group_names.begin(), group_names.end(), OPTIONS[i].name) == group_names.end())
      group_names.push_back(OPTIONS[i].name);
  }

  auto format_option = [&](const Option &opt) {
    std::string f = std::string("+ **-") + opt.id;
    for (const Argument *leaf : opt.leaves())
      f += std::string(" ") + leaf->id;
    f += "**";
    if (opt.flags.allow_multiple())
      f += "  *(multiple uses permitted)*";
    f += std::string("<br>") + opt.desc + "\n\n";
    // Tuple sub-arguments listed as an indented sub-list beneath the option.
    for (const auto &arg : opt)
      if (arg.is_tuple())
        for (const auto &element : arg.elements)
          if (!element.desc.empty())
            f += std::string("    - *") + element.id + "*: " + element.desc + "\n";
    return f;
  };

  s += "\n## Options\n\n";
  for (size_t i = 0; i < group_names.size(); ++i) {
    size_t n = i;
    while (OPTIONS[n].name != group_names[i])
      ++n;
    if (OPTIONS[n].name != std::string("OPTIONS"))
      s += std::string("#### ") + OPTIONS[n].name + "\n\n";
    while (n < OPTIONS.size()) {
      if (OPTIONS[n].name == group_names[i]) {
        for (size_t o = 0; o < OPTIONS[n].size(); ++o)
          s += format_option(OPTIONS[n][o]);
      }
      ++n;
    }
  }

  s += "#### Standard options\n\n";
  for (size_t i = 0; i < _standard_options.size(); ++i)
    s += format_option(_standard_options[i]);

  s += std::string("## References\n\n");
  for (size_t i = 0; i < REFERENCES.size(); ++i)
    s += std::string(REFERENCES[i]) + "\n\n";
  s += core_reference + "\n\n";

  s += std::string("**Author:** ") + AUTHOR + "\n\n";
  s += std::string("**Copyright:** ") + COPYRIGHT + "\n\n";

  // Append one complete page per sub-interface, in declaration order. Iterate over a copy,
  //   since render_subcommand() clears and restores the global SUBCOMMANDS while rendering.
  if (hierarchical) {
    const SubcommandList subcommands = SUBCOMMANDS;
    for (const auto &sub : subcommands)
      s += render_subcommand(sub, ExportFormat::Markdown);
  }

  return s;
}

std::string restructured_text_usage() {
  /*
    help_head (format)
    + help_synopsis (format)
    + usage_syntax (format)
    + ARGUMENTS.syntax (format)
    + DESCRIPTION.syntax (format)
    + EXAMPLES.syntax (format)
    + OPTIONS.syntax (format)
    + _standard_options.header (format)
    + _standard_options.contents (format)
    + _standard_options.footer (format)
    + help_tail (format);
  */

  const bool hierarchical = !SUBCOMMANDS.empty();
  std::string s = std::string("Synopsis\n--------\n\n") + SYNOPSIS + "\n\n";

  // Will need more sophisticated escaping of special characters
  //   if they start popping up in argument / option descriptions
  auto escape_special = [](std::string text) {
    size_t index = 0;
    while ((index = text.find("|", index)) != std::string::npos) {
      text.replace(index, 1, "\\|");
      index += 2;
    }
    return text;
  };

  if (hierarchical) {
    // A hierarchical command presents the sub-interface selection in place of any
    //   positional arguments (of which the top-level command has none).
    s += "Usage\n--------\n\n::\n\n    " + std::string(NAME) + " " + SUBCOMMANDS_SELECTOR + " [ options ] ...\n\n";
    s += std::string("-  *") + SUBCOMMANDS_SELECTOR + "*: " + selection_help_string() + "\n\n";
  } else {
    s += "Usage\n--------\n\n::\n\n    " + std::string(NAME) + " [ options ] ";

    // Syntax line:
    for (size_t i = 0; i < ARGUMENTS.size(); ++i) {

      if (ARGUMENTS[i].flags.optional())
        s += "[";
      s += std::string(" ") + ARGUMENTS[i].syntax_id();

      if (ARGUMENTS[i].flags.allow_multiple()) {
        if (ARGUMENTS[i].flags.required())
          s += std::string(" [ ") + ARGUMENTS[i].syntax_id();
        s += " ...";
      }
      if (ARGUMENTS[i].flags.any())
        s += " ]";
    }
    s += "\n\n";

    // Argument description (a tuple's sub-arguments follow on |br| continuation lines):
    for (size_t i = 0; i < ARGUMENTS.size(); ++i) {
      auto desc = split_lines(escape_special(ARGUMENTS[i].desc), false);
      s += std::string("-  *") + ARGUMENTS[i].syntax_id() + "*: " + desc[0];
      for (size_t n = 1; n < desc.size(); ++n)
        s += " |br|\n   " + desc[n];
      for (const auto &element : ARGUMENTS[i].elements) {
        if (element.desc.empty())
          continue;
        auto edesc = split_lines(escape_special(element.desc), false);
        s += " |br|\n   *" + element.id + "*: " + edesc[0];
        for (size_t n = 1; n < edesc.size(); ++n)
          s += " |br|\n   " + edesc[n];
      }
      s += "\n";
    }
    s += "\n";
  }

  if (!DESCRIPTION.empty()) {
    s += "Description\n-----------\n\n";
    for (size_t i = 0; i < DESCRIPTION.size(); ++i) {
      auto desc = split_lines(DESCRIPTION[i], false);
      s += desc[0];
      for (size_t n = 1; n < desc.size(); ++n)
        s += " |br|\n" + desc[n];
      s += "\n\n";
    }
  }

  if (!EXAMPLES.empty()) {
    s += "Example usages\n--------------\n\n";
    for (size_t i = 0; i < EXAMPLES.size(); ++i) {
      s += std::string("-   *") + EXAMPLES[i].title + "*::\n\n";
      s += std::string("        $ ") + EXAMPLES[i].code + "\n\n";
      if (!EXAMPLES[i].description.empty())
        s += std::string("    ") + EXAMPLES[i].description + "\n\n";
    }
  }

  std::vector<std::string> group_names;
  for (size_t i = 0; i < OPTIONS.size(); ++i) {
    if (std::find(group_names.begin(), group_names.end(), OPTIONS[i].name) == group_names.end())
      group_names.push_back(OPTIONS[i].name);
  }

  auto format_option = [&](const Option &opt) {
    std::string f = std::string("-  **-") + opt.id;
    for (const Argument *leaf : opt.leaves())
      f += std::string(" ") + leaf->id;
    f += std::string("** ");
    if (opt.flags.allow_multiple())
      f += "*(multiple uses permitted)* ";
    auto desc = split_lines(opt.desc, false);
    f += escape_special(desc[0]);
    for (size_t n = 1; n < desc.size(); ++n)
      f += " |br|\n   " + escape_special(desc[n]);
    // A tuple's sub-arguments follow on |br| continuation lines.
    for (const auto &arg : opt) {
      if (!arg.is_tuple())
        continue;
      for (const auto &element : arg.elements) {
        if (element.desc.empty())
          continue;
        auto edesc = split_lines(escape_special(element.desc), false);
        f += " |br|\n   *" + element.id + "*: " + edesc[0];
        for (size_t n = 1; n < edesc.size(); ++n)
          f += " |br|\n   " + edesc[n];
      }
    }
    f += "\n\n";
    return f;
  };

  s += "Options\n-------\n\n";
  for (size_t i = 0; i < group_names.size(); ++i) {
    size_t n = i;
    while (OPTIONS[n].name != group_names[i])
      ++n;
    if (OPTIONS[n].name != std::string("OPTIONS"))
      s += OPTIONS[n].name + std::string("\n") + std::string(OPTIONS[n].name.size(), '^') + "\n\n";
    while (n < OPTIONS.size()) {
      if (OPTIONS[n].name == group_names[i]) {
        for (size_t o = 0; o < OPTIONS[n].size(); ++o)
          s += format_option(OPTIONS[n][o]);
      }
      ++n;
    }
  }

  s += "Standard options\n^^^^^^^^^^^^^^^^\n\n";
  for (size_t i = 0; i < _standard_options.size(); ++i)
    s += format_option(_standard_options[i]);

  s += std::string("References\n^^^^^^^^^^\n\n");
  for (size_t i = 0; i < REFERENCES.size(); ++i) {
    auto refs = split_lines(REFERENCES[i], false);
    s += refs[0];
    for (size_t n = 1; n < refs.size(); ++n)
      s += " |br|\n  " + refs[n];
    s += "\n\n";
  }
  s += core_reference + "\n\n";

  s += std::string("--------------\n\n") + "\n\n**Author:** " + AUTHOR + "\n\n**Copyright:** " + COPYRIGHT + "\n\n";

  // Append one complete section per sub-interface, in declaration order; each section
  //   carries its own RST label and title (the top-level command's own label and title
  //   are added by the documentation generator, matching every C++ command).
  if (hierarchical) {
    const SubcommandList subcommands = SUBCOMMANDS;
    for (const auto &sub : subcommands)
      s += subcommand_rst_section(sub);
  }

  return s;
}

namespace {

//! render one sub-interface's export page in isolation, as an ordinary command page
/*! The sub-interface is installed over the globals (with its NAME set to
 *  "<command> <sub-interface>") and SUBCOMMANDS temporarily cleared so the renderer
 *  treats it as an ordinary, non-hierarchical command; all globals are restored on
 *  return. */
std::string render_subcommand(const Subcommand &sub, const ExportFormat format) {
  const std::string saved_synopsis = SYNOPSIS;
  const std::string saved_author = AUTHOR;
  const std::string saved_copyright = COPYRIGHT;
  const Description saved_description = DESCRIPTION;
  const ExampleList saved_examples = EXAMPLES;
  const ArgumentList saved_arguments = ARGUMENTS;
  const OptionList saved_options = OPTIONS;
  const Description saved_references = REFERENCES;
  const std::string saved_name = NAME;
  const SubcommandList saved_subcommands = SUBCOMMANDS;

  install_subcommand(sub);
  SUBCOMMANDS = SubcommandList();
  NAME = saved_name + " " + sub.id;

  std::string out;
  switch (format) {
  case ExportFormat::FullUsage:
    out = full_usage();
    break;
  case ExportFormat::Markdown:
    out = markdown_usage();
    break;
  case ExportFormat::Rst:
    out = restructured_text_usage();
    break;
  }

  SYNOPSIS = saved_synopsis;
  AUTHOR = saved_author;
  COPYRIGHT = saved_copyright;
  DESCRIPTION = saved_description;
  EXAMPLES = saved_examples;
  ARGUMENTS = saved_arguments;
  OPTIONS = saved_options;
  REFERENCES = saved_references;
  NAME = saved_name;
  SUBCOMMANDS = saved_subcommands;
  return out;
}

//! a sub-interface's RST section: its own label and title, then its full page
std::string subcommand_rst_section(const Subcommand &sub) {
  const std::string title = std::string(NAME) + " " + sub.id;
  std::string s = ".. _" + std::string(NAME) + "_" + sub.id + ":\n\n";
  s += title + "\n" + std::string(title.size(), '=') + "\n\n";
  s += render_subcommand(sub, ExportFormat::Rst);
  return s;
}

} // namespace

const Option *match_option(std::string_view arg) {
  auto no_dash_arg = without_leading_dash(arg);
  if (arg.size() == no_dash_arg.size() || no_dash_arg.empty() || isdigit(no_dash_arg.front()) != 0 ||
      no_dash_arg.front() == '.') {
    return nullptr;
  }

  std::vector<const Option *> candidates;
  std::string root(no_dash_arg);

  for (size_t i = 0; i < OPTIONS.size(); ++i)
    get_matches(candidates, OPTIONS[i], root);
  get_matches(candidates, _standard_options, root);

  // no matches
  if (candidates.empty())
    throw Exception(std::string("unknown option \"-") + root + "\"");

  // return match if unique:
  if (candidates.size() == 1)
    return candidates[0];

  // return match if fully specified:
  for (size_t i = 0; i < candidates.size(); ++i)
    if (root == candidates[i]->id)
      return candidates[i];

  // check if there is only one *unique* candidate
  const auto cid = candidates[0]->id;
  if (std::all_of(++candidates.begin(), candidates.end(), [&cid](const Option *cand) { return cand->id == cid; }))
    return candidates[0];

  // report something useful:
  root = "several matches possible for option \"-" + root + "\": \"-" + candidates[0]->id;

  for (size_t i = 1; i < candidates.size(); ++i)
    root += std::string("\", \"-") + candidates[i]->id + "\"";

  throw Exception(root);
}

void sort_arguments(const std::vector<std::string> &arguments) {
  auto it = arguments.begin();
  while (it != arguments.end()) {
    const size_t index = std::distance(arguments.begin(), it);
    const Option *opt = match_option(*it);
    if (opt != nullptr) {
      if (it + opt->arity() >= arguments.end()) {
        throw Exception(std::string("not enough parameters to option \"-") + opt->id + "\"");
      }

      std::vector<std::string> option_args;
      std::copy_n(it + 1, opt->arity(), std::back_inserter(option_args));
      option.push_back(ParsedOption(opt, option_args, index));
      it += opt->arity();
    } else {
      argument.push_back(ParsedArgument(nullptr, nullptr, *it, index));
    }
    ++it;
  }
}

void parse_standard_options() {
  if (!get_options("info").empty())
    log_level = std::max(log_level, 2);
  if (!get_options("debug").empty())
    log_level = 3;
  if (!get_options("quiet").empty())
    log_level = 0;
  if (!get_options("force").empty()) {
    WARN("existing output files will be overwritten");
    overwrite_files = true;
  }
}

void verify_usage() {
  if (AUTHOR.empty())
    throw Exception("No author specified for command " + std::string(NAME));
  if (SYNOPSIS.empty())
    throw Exception("No synopsis specified for command " + std::string(NAME));
  if (!SUBCOMMANDS.empty()) {
    if (!ARGUMENTS.empty())
      throw Exception("A hierarchical command must not declare top-level ARGUMENTS"
                      " (the sub-interface selection is the sole leading positional)");
    if (SUBCOMMANDS_SELECTOR.empty())
      throw Exception("A hierarchical command must specify a non-empty SUBCOMMANDS_SELECTOR");
    for (const auto &sub : SUBCOMMANDS) {
      if (sub.id.empty())
        throw Exception("A sub-interface of command " + std::string(NAME) + " has no name");
      if (sub.synopsis.empty())
        throw Exception("Sub-interface \"" + sub.id + "\" of command " + std::string(NAME) + " has no synopsis");
      if (SUBCOMMANDS.find(sub.id) != &sub)
        throw Exception("Duplicate sub-interface name \"" + sub.id + "\" in command " + std::string(NAME));
    }
  }
}

namespace {

//! emit a top-level machine-readable export for the current (possibly hierarchical) command
[[noreturn]] void emit_top_level_export(const ExportFormat format, const bool synopsis) {
  if (synopsis)
    print(SYNOPSIS);
  else
    switch (format) {
    case ExportFormat::FullUsage:
      print(full_usage());
      break;
    case ExportFormat::Markdown:
      print(markdown_usage());
      break;
    case ExportFormat::Rst:
      print(restructured_text_usage());
      break;
    }
  throw 0;
}

} // namespace

void parse_special_options() {
  if (raw_arguments_list.empty())
    return;

  bool synopsis = false;
  ExportFormat format = ExportFormat::FullUsage;
  const std::string_view last = raw_arguments_list.back();
  if (last == "__print_full_usage__") {
    format = ExportFormat::FullUsage;
  } else if (last == "__print_usage_markdown__") {
    format = ExportFormat::Markdown;
  } else if (last == "__print_usage_rst__") {
    format = ExportFormat::Rst;
  } else if (last == "__print_synopsis__") {
    synopsis = true;
  } else {
    return;
  }

  if (!SUBCOMMANDS.empty()) {
    // A per-sub-interface export is requested as "<command> <sub-interface> <keyword>";
    //   any other keyword-terminated form (including __print_synopsis__) emits the
    //   top-level page. This mirrors the Python parser's dispatch on the second-to-last
    //   command-line token.
    if (!synopsis && raw_arguments_list.size() >= 2) {
      const Subcommand *sub = SUBCOMMANDS.find(raw_arguments_list[raw_arguments_list.size() - 2]);
      if (sub != nullptr) {
        // The rst form carries the sub-interface's own label and title (a complete section);
        //   markdown / full_usage emit the sub-interface page alone.
        print(format == ExportFormat::Rst ? subcommand_rst_section(*sub) : render_subcommand(*sub, format));
        throw 0;
      }
    }
    emit_top_level_export(format, synopsis);
  }

  // Non-hierarchical commands: the keyword is special only as the solitary argument.
  if (raw_arguments_list.size() != 1)
    return;
  emit_top_level_export(format, synopsis);
}

//! select and install a hierarchical command's sub-interface from the command-line
/*! Identifies the selection token (the first token that is neither an option nor an
 *  argument consumed by a preceding option, resolved against the command's common and
 *  standard options), installs the selected sub-interface over the usage() globals, and
 *  removes the selection token from the argument list so that ordinary parsing proceeds
 *  on the sub-interface. Handles -help / -version (routed to the sub-interface when one
 *  is selected, else to the top-level command) and the no-/unknown-selection errors. */
void select_subcommand() {
  capture_top_level_interface();
  const std::vector<std::string> ids = SUBCOMMANDS.ids();

  // A completely empty command-line prints the top-level help page (as every command does).
  if (raw_arguments_list.empty()) {
    if (REQUIRES_AT_LEAST_ONE_ARGUMENT) {
      print_help();
      throw 0;
    }
    throw Exception("no algorithm selected (expected one of: " + join(ids, ", ") + ")");
  }

  // Locate the selection token: the first token that is not an option nor an option's
  //   argument. match_option() throws on an unrecognised dashed token (e.g. a
  //   sub-interface-specific option placed before the selection), which propagates as the
  //   parse error, matching the Python parser.
  std::optional<std::string> selection;
  std::optional<size_t> selection_index;
  for (size_t i = 0; i < raw_arguments_list.size();) {
    const Option *opt = match_option(raw_arguments_list[i]);
    if (opt == nullptr) {
      selection = raw_arguments_list[i];
      selection_index = i;
      break;
    }
    i += 1 + opt->arity();
  }

  const Subcommand *sub = selection.has_value() ? SUBCOMMANDS.find(*selection) : nullptr;

  if (sub != nullptr) {
    install_subcommand(*sub);
    SUBCOMMAND_SELECTED_ID = sub->id;
    SUBCOMMANDS = SubcommandList();
    raw_arguments_list.erase(raw_arguments_list.begin() + *selection_index);
    // Route -help / -version to the selected sub-interface, giving per-sub-interface help.
    const HelpVersion help_version = prescan_help_version(raw_arguments_list);
    if (help_version == HelpVersion::Help) {
      NAME = std::string(NAME) + " " + SUBCOMMAND_SELECTED_ID;
      print_help();
      throw 0;
    }
    if (help_version == HelpVersion::Version) {
      NAME = std::string(NAME) + " " + SUBCOMMAND_SELECTED_ID;
      print(version_string());
      throw 0;
    }
    return;
  }

  // No valid sub-interface selected: -help / -version are routed to the top-level command,
  //   taking precedence over the no-/unknown-selection errors.
  const HelpVersion help_version = prescan_help_version(raw_arguments_list);
  if (help_version == HelpVersion::Help) {
    print_help();
    throw 0;
  }
  if (help_version == HelpVersion::Version) {
    print(version_string());
    throw 0;
  }
  if (!selection.has_value())
    throw Exception("no algorithm selected (expected one of: " + join(ids, ", ") + ")");
  throw Exception("unknown algorithm \"" + *selection + "\" (expected one of: " + join(ids, ", ") + ")");
}

void parse() {
  argument.clear();
  option.clear();

  if (!SUBCOMMANDS.empty())
    select_subcommand();

  sort_arguments(raw_arguments_list);

  if (!get_options("help").empty()) {
    print_help();
    throw 0;
  }
  if (!get_options("version").empty()) {
    print(version_string());
    throw 0;
  }

  // Positional arguments may be tuples (arity > 1) as well as optional / repeatable. Counting
  //   is performed in tokens: each ARGUMENTS entry contributes arity() required tokens, except
  //   any optional / repeatable entry, which absorbs surplus tokens in whole groups of its
  //   arity. This reduces exactly to the scalar behaviour when every arity is 1.
  size_t num_required_tokens = 0;
  size_t num_optional_slots = 0;
  size_t flagged_arity = 0;

  ArgModifierFlags flags;
  for (size_t i = 0; i < ARGUMENTS.size(); ++i) {
    const size_t slot_arity = ARGUMENTS[i].arity();
    if (ARGUMENTS[i].flags.any()) {
      if (flags.any() && (flags != ARGUMENTS[i].flags || flagged_arity != slot_arity))
        throw Exception("FIXME: all arguments declared optional() or allow_multiple()"
                        " should have matching flags in command-line syntax");
      flags = ARGUMENTS[i].flags;
      flagged_arity = slot_arity;
      ++num_optional_slots;
      if (!flags.optional())
        num_required_tokens += slot_arity;
    } else
      num_required_tokens += slot_arity;
  }

  if (option.empty() && argument.empty() && REQUIRES_AT_LEAST_ONE_ARGUMENT) {
    print_help();
    throw 0;
  }

  if (num_optional_slots && num_required_tokens > argument.size())
    throw Exception("Expected at least " + str(num_required_tokens) + " arguments (" + str(argument.size()) +
                    " supplied)");

  if (num_optional_slots == 0 && num_required_tokens != argument.size()) {
    Exception e("Expected exactly " + str(num_required_tokens) + " arguments (" + str(argument.size()) + " supplied)");
    std::string s = "Usage: " + NAME;
    for (const auto &a : ARGUMENTS)
      s += " " + a.syntax_id();
    e.push_back(s);
    s = "Yours: " + NAME;
    for (const auto &a : argument)
      s += " " + std::string(a);
    e.push_back(s);
    if (argument.size() > num_required_tokens) {
      std::vector<std::string> potential_options;
      for (const auto &a : argument) {
        for (const auto &og : OPTIONS) {
          for (const auto &o : og) {
            if (std::string(a) == std::string(o.id))
              potential_options.push_back("'-" + std::string(a) + "'");
          }
        }
      }
      if (!potential_options.empty())
        e.push_back("(Did you mean " + join(potential_options, " or ") + "?)");
    }
    throw e;
  }

  const size_t group = flagged_arity != 0 ? flagged_arity : 1;
  const size_t num_extra_tokens = argument.size() - num_required_tokens;
  const size_t num_extra_groups = num_optional_slots != 0 ? num_extra_tokens / (num_optional_slots * group) : 0;
  if (num_optional_slots != 0 && num_extra_groups * num_optional_slots * group != num_extra_tokens)
    throw Exception("number of optional arguments provided are not equal for all arguments");
  size_t tokens_per_optional_slot = num_extra_groups * group;
  if (num_optional_slots != 0 && flags.required())
    tokens_per_optional_slot += group;

  // assign each parsed positional token to its corresponding (leaf) Argument definition; a
  //   tuple positional maps consecutive tokens to its sub-arguments in cyclic order.
  {
    size_t n = 0;
    for (size_t i = 0; i < ARGUMENTS.size(); ++i) {
      const Argument &slot = ARGUMENTS[i];
      const size_t slot_tokens = slot.flags.any() ? tokens_per_optional_slot : slot.arity();
      for (size_t t = 0; t < slot_tokens; ++t, ++n)
        argument[n].arg = slot.is_tuple() ? &slot.elements[t % slot.arity()] : &slot;
    }
    assert(n == argument.size());
  }

  // check for multiple instances of options:
  for (size_t i = 0; i < OPTIONS.size(); ++i) {
    for (size_t j = 0; j < OPTIONS[i].size(); ++j) {
      size_t count = 0;
      for (size_t k = 0; k < option.size(); ++k)
        if (option[k].opt == &OPTIONS[i][j])
          count++;

      if (count < 1 && OPTIONS[i][j].flags.required())
        throw Exception(std::string("mandatory option \"-") + OPTIONS[i][j].id + "\" must be specified");

      if (count > 1 && !OPTIONS[i][j].flags.allow_multiple())
        throw Exception(std::string("multiple instances of option \"-") + OPTIONS[i][j].id + "\" are not allowed");
    }
  }

  parse_standard_options();

  File::Config::init();

  // CONF option: FailOnWarn
  // CONF default: 0 (false)
  // CONF A boolean value specifying whether MRtrix applications should
  // CONF abort as soon as any (otherwise non-fatal) warning is issued.
  fail_on_warn = File::Config::get_bool("FailOnWarn", false);

  // CONF option: TerminalColor
  // CONF default: 1 (true)
  // CONF A boolean value to indicate whether colours should be used in the terminal.
  terminal_use_colour = File::Config::get_bool("TerminalColor", terminal_use_colour);

  // check for the existence of all specified input files (including optional ones that have been provided)
  // if necessary, also check for pre-existence of any output files or directories with known paths
  // note that if an argument has multiple possible types, some checks can't be enforced
  for (const auto &i : argument) {
    assert(i.arg->types.any());
    {
      ArgTypeFlags types_not_input_file(i.arg->types);
      types_not_input_file.reset(ArgTypeFlags::FileIn);
      types_not_input_file.reset(ArgTypeFlags::TracksIn);
      if (!types_not_input_file.any()) {
        if (!std::filesystem::exists(i))
          throw Exception("required input file \"" + i.as_text() + "\" not found");
        if (!std::filesystem::is_regular_file(i))
          throw Exception("required input \"" + i.as_text() + "\" is not a file");
      }
    }
    {
      ArgTypeFlags types_not_input_directory(i.arg->types);
      types_not_input_directory.reset(ArgTypeFlags::DirectoryIn);
      if (!types_not_input_directory.any()) {
        if (!std::filesystem::exists(i))
          throw Exception("required input directory \"" + i.as_text() + "\" not found");
        if (!std::filesystem::is_directory(i))
          throw Exception("required input \"" + i.as_text() + "\" is not a directory");
      }
    }
    {
      ArgTypeFlags types_not_output_file(i.arg->types);
      types_not_output_file.reset(ArgTypeFlags::FileOut);
      types_not_output_file.reset(ArgTypeFlags::TracksOut);
      if (!types_not_output_file.any()) {
        if (i.as_text().find_last_of(PATH_SEPARATORS) == i.as_text().size() - 1)
          throw Exception("output path \"" + i.as_text() + "\" is not a valid file path" +
                          " (ends with directory path separator)");
      }
    }
    {
      ArgTypeFlags types_not_output_filesystem(i.arg->types);
      types_not_output_filesystem.reset(ArgTypeFlags::FileOut);
      types_not_output_filesystem.reset(ArgTypeFlags::DirectoryOut);
      types_not_output_filesystem.reset(ArgTypeFlags::TracksOut);
      if (!types_not_output_filesystem.any()) {
        if (i.arg->types[ArgTypeFlags::DirectoryOut] && !i.arg->types[ArgTypeFlags::FileOut] &&
            !i.arg->types[ArgTypeFlags::TracksOut]) {
          switch (i.arg->dir_out_mode) {
          case DirOutMode::MustNotExist:
            check_overwrite(i);
            break;
          case DirOutMode::EmptyOrAbsent: {
            const std::filesystem::path dir_path(i);
            if (std::filesystem::exists(dir_path)) {
              if (!std::filesystem::is_directory(dir_path))
                throw Exception("output path \"" + i.as_text() + "\" already exists as a file");
              if (std::filesystem::directory_iterator(dir_path) != std::filesystem::directory_iterator())
                throw Exception("output directory \"" + i.as_text() + "\" is not empty" +
                                (overwrite_files ? " (-force option cannot safely be applied on directories;"
                                                   " please erase manually instead)"
                                                 : ""));
            }
            break;
          }
          case DirOutMode::MayExist:
            break;
          }
        } else {
          check_overwrite(i);
        }
      }
    }
    {
      ArgTypeFlags types_not_input_tractogram(i.arg->types);
      types_not_input_tractogram.reset(ArgTypeFlags::TracksIn);
      if (!types_not_input_tractogram.any()) {
        if (static_cast<std::filesystem::path>(i).extension() != ".tck")
          throw Exception("input file \"" + i.as_text() + "\" is not a valid track file");
      }
    }
    {
      ArgTypeFlags types_not_output_tractogram(i.arg->types);
      types_not_output_tractogram.reset(ArgTypeFlags::TracksOut);
      if (!types_not_output_tractogram.any()) {
        if (static_cast<std::filesystem::path>(i).extension() != ".tck")
          throw Exception("output track file \"" + i.as_text() + "\" must use the .tck suffix");
      }
    }
  }
  for (const auto &i : option) {
    const std::vector<const Argument *> leaves = i.opt->leaves();
    for (size_t j = 0; j != leaves.size(); ++j) {
      const ParsedArgument parg = i[j];
      const Argument &arg = *leaves[j];
      assert(arg.types.any());
      {
        ArgTypeFlags types_not_input_file(arg.types);
        types_not_input_file.reset(ArgTypeFlags::FileIn);
        types_not_input_file.reset(ArgTypeFlags::TracksIn);
        if (!types_not_input_file.any()) {
          if (!std::filesystem::exists(parg))
            throw Exception("input file \"" + parg.as_text() + "\"" +                     //
                            " for option \"-" + std::string(i.opt->id) + "\" not found"); //
          if (!std::filesystem::is_regular_file(parg))
            throw Exception("input \"" + parg.as_text() + "\"" +                              //
                            " for option \"-" + std::string(i.opt->id) + "\" is not a file"); //
        }
      }
      {
        ArgTypeFlags types_not_input_directory(arg.types);
        types_not_input_directory.reset(ArgTypeFlags::DirectoryIn);
        if (!types_not_input_directory.any()) {
          if (!std::filesystem::exists(parg))
            throw Exception("input directory \"" + parg.as_text() + "\"" +                //
                            " for option \"-" + std::string(i.opt->id) + "\" not found"); //
          if (!std::filesystem::is_directory(parg))
            throw Exception("input \"" + parg.as_text() + "\"" +                                   //
                            " for option \"-" + std::string(i.opt->id) + "\" is not a directory"); //
        }
      }
      {
        ArgTypeFlags types_not_output_file(arg.types);
        types_not_output_file.reset(ArgTypeFlags::FileOut);
        types_not_output_file.reset(ArgTypeFlags::TracksOut);
        if (!types_not_output_file.any()) {
          const std::string filename = static_cast<std::filesystem::path>(parg).filename().string();
          if (filename.find_last_of(PATH_SEPARATORS) == filename.size() - 1)
            throw Exception("output path \"" + parg.as_text() + "\"" +                         //
                            " for option \"-" + std::string(i.opt->id) + "\"" +                //
                            " is not a valid file path (ends with directory path separator)"); //
        }
      }
      {
        ArgTypeFlags types_not_output_filesystem(arg.types);
        types_not_output_filesystem.reset(ArgTypeFlags::FileOut);
        types_not_output_filesystem.reset(ArgTypeFlags::DirectoryOut);
        types_not_output_filesystem.reset(ArgTypeFlags::TracksOut);
        if (!types_not_output_filesystem.any()) {
          if (arg.types[ArgTypeFlags::DirectoryOut] && !arg.types[ArgTypeFlags::FileOut] &&
              !arg.types[ArgTypeFlags::TracksOut]) {
            switch (arg.dir_out_mode) {
            case DirOutMode::MustNotExist:
              check_overwrite(parg);
              break;
            case DirOutMode::EmptyOrAbsent: {
              const std::filesystem::path dir_path(parg);
              if (std::filesystem::exists(dir_path)) {
                if (!std::filesystem::is_directory(dir_path))
                  throw Exception("output path \"" + parg.as_text() + "\"" + " for option \"-" +
                                  std::string(i.opt->id) + "\" already exists as a file");
                if (std::filesystem::directory_iterator(dir_path) != std::filesystem::directory_iterator())
                  throw Exception("output directory \"" + parg.as_text() + "\"" + " for option \"-" +
                                  std::string(i.opt->id) + "\" is not empty" +
                                  (overwrite_files ? " (-force option cannot safely be applied on directories;"
                                                     " please erase manually instead)"
                                                   : ""));
              }
              break;
            }
            case DirOutMode::MayExist:
              break;
            }
          } else {
            check_overwrite(parg);
          }
        }
      }
      {
        ArgTypeFlags types_not_input_tractogram(arg.types);
        types_not_input_tractogram.reset(ArgTypeFlags::TracksIn);
        if (!types_not_input_tractogram.any()) {
          if (static_cast<std::filesystem::path>(parg).extension() != ".tck")
            throw Exception("input file \"" + parg.as_text() + "\"" +           //
                            " for option \"-" + std::string(i.opt->id) + "\"" + //
                            " is not a valid track file");                      //
        }
      }
      {
        ArgTypeFlags types_not_output_tractogram(arg.types);
        types_not_output_tractogram.reset(ArgTypeFlags::TracksOut);
        if (!types_not_output_tractogram.any()) {
          if (static_cast<std::filesystem::path>(parg).extension() != ".tck")
            throw Exception("output track file \"" + parg.as_text() + "\"" +    //
                            " for option \"-" + std::string(i.opt->id) + "\"" + //
                            " must use the .tck suffix");                       //
        }
      }
    }
  }

  SignalHandler::init();
}

void init(int cmdline_argc, const char *const *cmdline_argv) { // check_syntax off
#ifdef MRTRIX_WINDOWS
  // force stderr to be unbuffered, and stdout to be line-buffered:
  setvbuf(stderr, nullptr, _IONBF, 0);
  setvbuf(stdout, nullptr, _IOLBF, 0);
#endif

  terminal_use_colour = !ProgressBar::set_update_method();

  raw_arguments_list = std::vector<std::string>(cmdline_argv, cmdline_argv + cmdline_argc);
  NAME = std::filesystem::path(raw_arguments_list.front()).filename().string();
  raw_arguments_list.erase(raw_arguments_list.begin());

#ifdef MRTRIX_WINDOWS
  if (std::filesystem::path(NAME).extension() == ".exe")
    NAME.erase(NAME.size() - 4);
#endif

  auto argv_quoted = [](std::string_view s) -> std::string {
    for (size_t i = 0; i != s.size(); ++i) {
      if (!(isalnum(s[i]) || s[i] == '.' || s[i] == '_' || s[i] == '-' || s[i] == '/')) {
        std::string escaped_string("\'");
        for (auto c : s) {
          switch (c) {
          case '\'':
            escaped_string.append("\\\'");
            break;
          case '\\':
            escaped_string.append("\\\\");
            break;
          default:
            escaped_string.push_back(c);
            break;
          }
        }
        escaped_string.push_back('\'');
        return escaped_string;
      }
    }
    return std::string(s);
  };
  command_history_string = cmdline_argv[0];
  for (const auto &a : raw_arguments_list)
    command_history_string += std::string(" ") + argv_quoted(a);
  command_history_string += std::string("  (version=") + mrtrix_version;
  if (!project_version.empty())
    command_history_string += std::string(", project=") + project_version;
  command_history_string += ")";

  std::locale::global(std::locale::classic());
  std::setlocale(LC_ALL, "C"); // NOLINT(concurrency-mt-unsafe)
}

std::vector<ParsedOption> get_options(std::string_view name) {
  assert(!name.empty());
  assert(name[0] != '-');
  std::vector<ParsedOption> matches;
  for (size_t i = 0; i < option.size(); ++i) {
    assert(option[i].opt);
    if (option[i].opt->is(name))
      matches.push_back({option[i].opt, option[i].args, option[i].index});
  }
  return matches;
}

int64_t App::ParsedArgument::as_int() const {

  std::string as_choice_msg;

  if (arg->types[ArgTypeFlags::Choice]) {
    const std::string selection = lowercase(p);
    auto it = std::find(arg->choices.begin(), arg->choices.end(), selection);
    if (it == arg->choices.end()) {
      std::string msg = std::string("unexpected value supplied for ");
      if (opt != nullptr)
        msg += std::string("option \"") + opt->id;
      else
        msg += std::string("argument \"") + arg->id;
      msg += std::string("\" ");
      as_choice_msg = std::string("received \"" + std::string(p) + "\"; ");
      as_choice_msg += std::string("valid choices are: ") + join(arg->choices, ", ");
      msg += "(" + as_choice_msg + ")";
      if (!arg->types[ArgTypeFlags::Integer])
        throw Exception(msg);
    } else {
      return static_cast<int>(std::distance(arg->choices.begin(), it));
    }
  }

  assert(arg->types[ArgTypeFlags::Integer]);

  // Check to see if there are any alpha characters in here
  // - If a single character at the end, use as integer multiplier
  //   - Unless there's a dot point before the multiplier; in which case,
  //     parse the number as a float, multiply, then cast to integer
  // - If a single 'e' or 'E' in the middle, parse as float and convert to integer
  std::string as_int_msg;
  try {
    size_t alpha_count = 0;
    bool alpha_is_last = false;
    bool contains_dotpoint = false;
    char alpha_char = 0;
    for (const char &c : p) {
      if (std::isalpha(c) != 0) {
        ++alpha_count;
        alpha_is_last = true;
        alpha_char = c;
      } else {
        alpha_is_last = false;
      }
      if (c == '.')
        contains_dotpoint = true;
    }
    if (alpha_count > 1)
      throw Exception("too many letters");
    int64_t retval = 0;
    if (alpha_count) {
      if (alpha_is_last) {
        std::string num(p);
        const char postfix = num.back();
        num.pop_back();
        int64_t multiplier = 1.0;
        switch (postfix) {
        case 'k':
        case 'K':
          multiplier = 1000;
          break;
        case 'm':
        case 'M':
          multiplier = 1000000;
          break;
        case 'b':
        case 'B':
          multiplier = 1000000000;
          break;
        case 't':
        case 'T':
          multiplier = 1000000000000;
          break;
        default:
          throw Exception(std::string("unexpected postfix \'") + postfix + "\'");
        }
        if (contains_dotpoint) {
          const default_type prefix = to<default_type>(num);
          retval = std::round(prefix * static_cast<default_type>(multiplier));
        } else {
          retval = to<int64_t>(num) * multiplier;
        }
      } else if (alpha_char == 'e' || alpha_char == 'E') {
        const default_type as_float = to<default_type>(p);
        retval = std::round(as_float);
      } else {
        throw Exception("unexpected character");
      }
    } else {
      retval = to<int64_t>(p);
    }

    if (retval < arg->int_limits.min() || retval > arg->int_limits.max())
      throw Exception(std::string("out of bounds")                     //
                      + " (valid range: " + str(arg->int_limits.min()) //
                      + " to " + str(arg->int_limits.max()) + ";"      //
                      + " value supplied: " + str(retval) + ")");      //
    return retval;
  } catch (Exception &e_int) {
    as_int_msg = e_int[0];
    if (!arg->types[ArgTypeFlags::Choice])
      throw Exception("unable to parse string " + str(p) + " supplied for " +
                      (opt == nullptr ? std::string("argument \"") + arg->id : std::string("option \"") + opt->id) +
                      " as integer: " + as_int_msg);
  }

  Exception full_msg(std::string("Unable to interpret value supplied for ") +
                     (opt == nullptr ? std::string("argument \"") + arg->id : std::string("option \"") + opt->id) +
                     " as either integer or choice selection");
  full_msg.push_back("Error when interpreted as choice selection:");
  full_msg.push_back(as_choice_msg);
  full_msg.push_back("Error when interpreted as integer:");
  full_msg.push_back(as_int_msg);
  throw full_msg;
}

bool App::ParsedArgument::as_bool() const {
  assert(arg->types[ArgTypeFlags::Boolean]);
  return to<bool>(p);
}

uint64_t App::ParsedArgument::as_uint() const {
  const int64_t signed_value = as_int();
  if (signed_value < 0)
    throw Exception("Attempting to interpret negative user-specified value (" //
                    + str(signed_value)                                       //
                    + " as unsigned integer");                                //
  return static_cast<uint64_t>(signed_value);
}

default_type App::ParsedArgument::as_float() const {
  assert(arg->types[ArgTypeFlags::Float]);
  const default_type retval = to<default_type>(p);
  if (retval < arg->float_limits.min() || retval > arg->float_limits.max()) {
    std::string msg("value supplied for ");
    if (opt)
      msg += std::string("option \"") + opt->id;
    else
      msg += std::string("argument \"") + arg->id;
    msg += "\" is out of bounds";
    msg += " (valid range: " + str(arg->float_limits.min()) + " to " + str(arg->float_limits.max()) + ";";
    msg += " value supplied: " + str(retval) + ")";
    throw Exception(msg);
  }

  return retval;
}

std::vector<ParsedArgument::IntType> ParsedArgument::as_sequence_int() const {
  assert(arg->types[ArgTypeFlags::IntSeq]);
  try {
    return parse_ints<IntType>(p);
  } catch (Exception &e) {
    throw Exception(e, "Unable to interpret command-line input \"" + as_text() + "\" as sequence of integers");
  }
}

std::vector<ParsedArgument::UIntType> ParsedArgument::as_sequence_uint() const {
  assert(arg->types[ArgTypeFlags::IntSeq]);
  try {
    return parse_ints<UIntType>(p);
  } catch (Exception &e) {
    throw Exception(e, "Unable to interpret command-line input \"" + as_text() + "\" as sequence of integers");
  }
}

std::vector<default_type> ParsedArgument::as_sequence_float() const {
  assert(arg->types[ArgTypeFlags::FloatSeq]);
  try {
    return parse_floats(p);
  } catch (Exception &e) {
    throw Exception(
        e, "Unable to interpret command-line input \"" + as_text() + "\" as sequence of floating-point values");
  }
}

ParsedArgument::ParsedArgument(const Option *option, const Argument *argument, std::string text, size_t index)
    : opt(option), arg(argument), p(std::move(text)), index_(index) {
  assert(!p.empty());
}

bool ParsedArgument::includes_filesystem_arg_types() const noexcept {
  if (arg == nullptr)
    return false;
  return (arg->types[ArgTypeFlags::FileIn] || arg->types[ArgTypeFlags::FileOut] ||
          arg->types[ArgTypeFlags::DirectoryIn] || arg->types[ArgTypeFlags::DirectoryOut] ||
          arg->types[ArgTypeFlags::ImageIn] || arg->types[ArgTypeFlags::ImageOut] ||
          arg->types[ArgTypeFlags::TracksIn] || arg->types[ArgTypeFlags::TracksOut]);
}

bool ParsedArgument::only_filesystem_arg_types() const noexcept {
  if (arg == nullptr)
    return false;
  ArgTypeFlags flags(arg->types);
  flags[ArgTypeFlags::FileIn] = flags[ArgTypeFlags::FileOut] = flags[ArgTypeFlags::DirectoryIn] =
      flags[ArgTypeFlags::DirectoryOut] = flags[ArgTypeFlags::ImageIn] = flags[ArgTypeFlags::ImageOut] =
          flags[ArgTypeFlags::TracksIn] = flags[ArgTypeFlags::TracksOut] = false;
  return !flags.any();
}

ParsedArgument::operator std::string() const {
  assert(!only_filesystem_arg_types());
  return p;
}

ParsedArgument::operator std::string_view() const {
  assert(!only_filesystem_arg_types());
  return p;
}

ParsedArgument::operator std::filesystem::path() const {
  assert(includes_filesystem_arg_types());
  return std::filesystem::path(p);
}

std::filesystem::path ParsedArgument::as_path() const {
  assert(includes_filesystem_arg_types());
  return std::filesystem::path(p);
}

void check_overwrite(const std::filesystem::path &path) {
  if (std::filesystem::exists(path) && !overwrite_files) {
    if (check_overwrite_files_func != nullptr)
      check_overwrite_files_func(path);
    else
      throw Exception("output path \"" + path.string() + "\" already exists (use -force option to force overwrite)");
  }
}

ParsedOption::ParsedOption(const Option *option, const std::vector<std::string> &arguments, size_t i)
    : opt(option), args(arguments), index(i) {
  const std::vector<const Argument *> leaves = option->leaves();
  for (size_t i = 0; i != leaves.size(); ++i) {
    const auto &p = arguments[i];
    if (!starts_with_dash(p))
      continue;
    if (leaves[i]->types[ArgTypeFlags::ImageIn] || leaves[i]->types[ArgTypeFlags::ImageOut] ||
        leaves[i]->types[ArgTypeFlags::Integer] || leaves[i]->types[ArgTypeFlags::Float] ||
        leaves[i]->types[ArgTypeFlags::IntSeq] || leaves[i]->types[ArgTypeFlags::FloatSeq])
      continue;
    WARN(std::string("Value \"") + arguments[i] + "\" is being used as " +
         ((leaves.size() == 1) ? "the expected argument "
                               : ("one of the " + str(leaves.size()) + " expected arguments ")) +
         "for option \"-" + option->id + "\"," + " yet this itself looks like a separate command-line option; " +
         "the requisite input" + ((leaves.size() == 1) ? " " : "s ") + "to command-line option \"-" + option->id +
         "\" may have been erroneously omitted, which may cause other command-line parsing errors");
  }
}

ParsedArgument ParsedOption::operator[](size_t num) const {
  const std::vector<const Argument *> leaves = opt->leaves();
  assert(num < leaves.size());
  return ParsedArgument(opt, leaves[num], args[num], index + num + 1);
}

ParsedArgument ParsedOption::operator[](std::string_view name) const {
  const std::vector<const Argument *> leaves = opt->leaves();
  for (size_t num = 0; num != leaves.size(); ++num)
    if (leaves[num]->id == name)
      return ParsedArgument(opt, leaves[num], args[num], index + num + 1);
  assert(false);
  throw Exception(std::string("Internal error: option \"-") + opt->id + "\" has no sub-argument named \"" +
                  std::string(name) + "\"");
}

bool ParsedOption::operator==(std::string_view match) const {
  const std::string name = lowercase(match);
  return name == opt->id;
}

std::string operator+(const char *const left, const ParsedArgument &right) { // check_syntax off
  std::string retval(left);
  retval += std::string(right);
  return retval;
}

std::ostream &operator<<(std::ostream &stream, const ParsedArgument &arg) {
  stream << std::string(arg);
  return stream;
}

} // namespace MR::App
