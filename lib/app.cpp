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

#include <gsl/gsl_version.h>

#include "app.h"
#include "debug.h"
#include "progressbar.h"
#include "file/path.h"
#include "file/config.h"

//#define NUM_DEFAULT_OPTIONS 5

namespace MR {

  namespace {

    const char* busy[] = {
      ".    ",
      " .   ",
      "  .  ",
      "   . ",
      "    .",
      "   . ",
      "  .  ",
      " .   "
    };



    void display_func_cmdline (ProgressInfo& p)
    {
      if (p.as_percentage) 
        fprintf (stderr, "\r%s: %s %3zu%%", App::name().c_str(), p.text.c_str(), size_t(p.value));
      else
        fprintf (stderr, "\r%s: %s %s", App::name().c_str(), p.text.c_str(), busy[p.value%8]);
    }


    void done_func_cmdline (ProgressInfo& p) 
    { 
      if (p.as_percentage)
        fprintf (stderr, "\r%s: %s %3u%%\n", App::name().c_str(), p.text.c_str(), 100); 
      else
        fprintf (stderr, "\r%s: %s  - ok\n", App::name().c_str(), p.text.c_str()); 
    }
  }





  void cmdline_print (const std::string& msg) { std::cout << msg; }

  void cmdline_error (const std::string& msg) 
  {
    if (App::log_level) std::cerr << App::name() << ": " << msg << "\n"; 
  }

  void cmdline_info  (const std::string& msg) 
  { 
    if (App::log_level > 1) std::cerr << App::name() << " [INFO]: " <<  msg << "\n"; 
  }

  void cmdline_debug (const std::string& msg)
  { 
    if (App::log_level > 2) std::cerr << App::name() << " [DEBUG]: " <<  msg << "\n"; 
  }





  namespace {

    void print_formatted_paragraph (const std::string& header, const std::string& text, int header_indent, int indent, int width)
    {
      int current = fprintf (stderr, "%-*s%-*s", header_indent, "", indent-header_indent-1, header.c_str());

      std::string::size_type start = 0, end;
      bool newline = false;
      do {
        end = start;
        while (!isspace(text [end]) && end < text.size()) end++;
        std::string token (text.substr (start, end-start));
        if (newline || current + (int) token.size() + 1 >= width) 
          current = fprintf (stderr, "\n%*s%s", indent, "", token.c_str()) - 1;
        else current += fprintf (stderr, " %s", token.c_str());
        newline = text[end] == '\n';
        start = end + 1;
      } while (end < text.size());
      fprintf (stderr, "\n");
    }

  }


#define HELP_WIDTH  80


#define HELP_PURPOSE_INDENT 0, 4, HELP_WIDTH
#define HELP_ARG_INDENT 8, 20, HELP_WIDTH
#define HELP_OPTION_INDENT 2, 20, HELP_WIDTH
#define HELP_OPTION_ARG_INDENT 20, 24, HELP_WIDTH



  int App::log_level = 1;
  std::string     App::application_name;
  const char**    App::command_description = NULL;
  const Argument* App::command_arguments = NULL;
  const Option*   App::command_options = NULL;
  const size_t*   App::version = NULL;
  const char*     App::copyright = NULL;
  const char*     App::author = NULL;

  const Option App::default_options[] = {
    Option ("info", "display information messages."),
    Option ("quiet", "do not display information messages or progress status."),
    Option ("debug", "display debugging messages."),
    Option ("help", "display this information page and exit."),
    Option ("version", "display version information and exit."),
    Option()
  };




  const Option* App::match_option (const char* stub) const
  {
    std::vector<const Option*> candidates;
    std::string root (stub);

    for (const Option* opt = command_options; *opt; ++opt)
      if (root.compare (0, root.size(), opt->id, root.size()) == 0)
        candidates.push_back (opt);

    for (const Option* opt = default_options; *opt; ++opt)
      if (root.compare (0, root.size(), opt->id, root.size()) == 0)
        candidates.push_back (opt);

    if (candidates.size() == 0) 
      return (NULL);

    if (candidates.size() == 1) 
      return (candidates[0]);

    for (std::vector<const Option*>::const_iterator opt = candidates.begin(); opt != candidates.end(); ++opt) 
      if (root == (*opt)->id)
        return (*opt);

    root = "several matches possible for option \"-" + root + "\": \"-" + candidates[0]->id;

    for (std::vector<const Option*>::const_iterator opt = candidates.begin()+1; opt != candidates.end(); ++opt) 
      root += std::string (", \"-") + (*opt)->id + "\"";

    throw Exception (root);
  }




  App::App (int argc, char** argv, const char** cmd_desc, const MR::Argument* cmd_args, const MR::Option* cmd_opts, 
      const size_t* cmd_version, const char* cmd_author, const char* cmd_copyright)
  {
#ifdef WINDOWS
    // force stderr to be unbuffered, and stdout to be line-buffered:
    setvbuf (stderr, NULL, _IONBF, 0);
    setvbuf (stdout, NULL, _IOLBF, 0);
#endif

    command_description = cmd_desc;
    command_arguments = cmd_args;
    command_options = cmd_opts;
    author = ( cmd_author ? cmd_author : "J-Donald Tournier (d.tournier@brain.org.au)" );
    version = cmd_version;
    copyright = ( cmd_copyright ? cmd_copyright : 
                "Copyright (C) 2008 Brain Research Institute, Melbourne, Australia.\n"
                "This is free software; see the source for copying conditions.\n"
                "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE." );

    if (argc == 2) {
      if (strcmp (argv[1], "__print_full_usage__") == 0) {
        print_full_usage ();
        throw (0);
      }
    }

    application_name = Path::basename (argv[0]);
#ifdef WINDOWS
    if (Path::has_suffix (application_name, ".exe")) 
      application_name.erase (application_name.size()-4);
#endif

    log_level = 1;

    ProgressBar::display_func = display_func_cmdline;
    ProgressBar::done_func = done_func_cmdline;

    print = cmdline_print;
    error = cmdline_error;
    info = cmdline_info;
    debug = cmdline_debug;


    sort_arguments (argc, argv); 

    srand (time (NULL));

    File::Config::init ();
  }


  App::~App () { }





  void App::sort_arguments (int argc, char** argv)
  {
    for (int n = 1; n < argc; n++) {
      const char* arg = argv[n];
      if (arg[0] == '-' && arg[1] && !isdigit(arg[1]) && arg[1] != '.') {

        while (*arg == '-') arg++;
        const Option* opt = match_option (arg);

        if (!opt) 
          throw Exception (std::string ("unknown option \"-") + arg + "\"");

        else if (opt == default_options) { // info
          if (log_level < 2) 
            log_level = 2;
        }
        else if (opt == default_options+1) { // quiet
          log_level = 0;
          ProgressBar::display = false;
        }
        else if (opt == default_options+2) { // debug
          log_level = 3;
        }
        else if (opt == default_options+3) { // help
          print_help ();
          throw 0;
        }
        else if (opt == default_options+4) { // version
          std::printf (
              "== %s %zu.%zu.%zu ==\n"
              "%d bit %s version, built " __DATE__ " against MRtrix %zu.%zu.%zu, using GSL %s\n"
              "Author: %s\n"
              "%s\n",

              App::name().c_str(), version[0], version[1], version[2], 
              int(8*sizeof(size_t)), 
#ifdef NDEBUG
              "release"
#else
              "debug"
#endif
              , mrtrix_major_version, mrtrix_minor_version, mrtrix_micro_version, 
              gsl_version, author, copyright);

          throw 0;
        }
        else { // command option
          if (n + int(opt->args.size()) >= argc) 
            throw Exception (std::string ("not enough parameters to option \"-") + opt->id + "\"");

          option.push_back (ParsedOption (opt, argv+n+1));
          n += opt->args.size();
        }
      }
      else argument.push_back (ParsedArgument (NULL, NULL, argv[n]));
    }
  }






  void App::parse_arguments ()
  {
    size_t num_args_required = 0, num_command_arguments = 0;
    bool has_optional_arguments = false;

    for (const Argument* arg = App::command_arguments; *arg; arg++) {
      num_command_arguments++;
      if (arg->flags & Optional) has_optional_arguments = true;
      else num_args_required++; 
      if (arg->flags & AllowMultiple) has_optional_arguments = true;
    }

    if (has_optional_arguments && num_args_required > argument.size()) 
      throw Exception ("expected at least " + str (num_args_required) 
          + " arguments (" + str(argument.size()) + " supplied)");

    if (!has_optional_arguments && num_args_required != argument.size()) 
      throw Exception ("expected exactly " + str (num_args_required) 
          + " arguments (" + str (argument.size()) + " supplied)");

    size_t optional_argument = std::numeric_limits<size_t>::max();
    for (size_t n = 0; n < argument.size(); n++) {

      if (n < optional_argument) 
        if (command_arguments[n].flags & (Optional | AllowMultiple) )
          optional_argument = n;

      size_t index = n;
      if (n >= optional_argument) {
        if (int(num_args_required - optional_argument) < int(argument.size()-n)) 
          index = optional_argument;
        else 
          index = num_args_required - argument.size() + n + (command_arguments[optional_argument].flags & Optional ? 1 : 0);
      }

      if (index >= num_command_arguments) 
        throw Exception ("too many arguments");

      argument[n].arg = command_arguments + index;
    }

    for (const Option* opt = command_options; *opt; ++opt) {
      size_t count = 0;
      for (std::vector<ParsedOption>::const_iterator popt = option.begin(); 
          popt != option.end(); ++popt)
        if (popt->opt == opt)
          count++;

      if (count < 1 && !(opt->flags & Optional)) 
        throw Exception (std::string ("mandatory option \"") + opt->id + "\" must be specified");

      if (count > 1 && !(opt->flags & AllowMultiple)) 
        throw Exception (std::string ("multiple instances of option \"") +  opt->id + "\" are not allowed");
    }

  }






  void App::print_help () const
  {
    fprintf (stderr, "%s: part of the MRtrix package\n\n", App::name().c_str());
    if (command_description[0]) {
      print_formatted_paragraph ("", command_description[0], HELP_PURPOSE_INDENT);
      fprintf (stderr, "\n");
      for (const char** p = command_description+1; *p; p++) {
        print_formatted_paragraph ("", *p, HELP_PURPOSE_INDENT);
        fprintf (stderr, "\n");
      }
    }
    else fprintf (stderr, "(no description available)\n\n");

    fprintf (stderr, "SYNTAX: %s [ options ]", App::name().c_str());
    for (const Argument* arg = command_arguments; *arg; ++arg) {

      if (arg->flags & Optional)
        fprintf (stderr, " [");

      fprintf (stderr, " %s", arg->id);

      if (arg->flags & AllowMultiple) {
        if (!(arg->flags & Optional))
          fprintf (stderr, " [ %s", arg->id);
        fprintf (stderr, " ...");
      }
      if (arg->flags & (Optional | AllowMultiple)) 
        fprintf (stderr, " ]");
    }
    fprintf (stderr, "\n\n");



    for (const Argument* arg = command_arguments; *arg; ++arg) 
      print_formatted_paragraph (std::string("- ") + arg->id + " ", arg->desc, HELP_ARG_INDENT);
    fprintf (stderr, "\n");


    fprintf (stderr, "OPTIONS:\n\n");
    for (const Option* opt = command_options; *opt; ++opt) {
      std::string text ("-");
      text += opt->id;

      for (std::vector<Argument>::const_iterator optarg = opt->args.begin(); 
          optarg != opt->args.end(); ++optarg) 
        text += std::string (" ") + optarg->id;
      
      print_formatted_paragraph (text + " ", opt->desc, HELP_OPTION_INDENT);
      // TODO: add argument defaults like this:
      //print_formatted_paragraph (text + " ", opt->desc + opt->arg_defaults(), HELP_OPTION_INDENT);

      fprintf (stderr, "\n");
    }

    fprintf (stderr, "Standard options:\n");
    for (const Option* opt = default_options; *opt; ++opt) 
      print_formatted_paragraph (std::string("-") + opt->id, opt->desc, HELP_OPTION_INDENT);
    fprintf (stderr, "\n");
  }







  void App::print_full_usage () const
  {
    for (const char** p = command_description; *p; p++) 
      std::cout << *p << "\n";

    for (const Argument* arg = command_arguments; arg; ++arg) 
      arg->print_usage();

    for (const Option* opt = command_options; opt; ++opt) 
      opt->print_usage();

    for (const Option* opt = default_options; opt; ++opt)
      opt->print_usage();
  }







  App::ParsedArgument::operator int () const 
  {
    if (arg->type == Integer) {
      const int retval = to<int> (p);
      const int min = arg->defaults.i.min;
      const int max = arg->defaults.i.max;
      if (retval < min || retval > max) {
        std::string msg ("value supplied for ");
        if (opt) msg += std::string ("option \"") + opt->id;
        else msg += std::string ("argument \"") + arg->id;
        msg += "\" is out of bounds (valid range: " + str(min) + " to " + str(max) + ", value supplied: " + str(retval) + ")";
        throw Exception (msg);
      }
      return retval;
    }

    if (arg->type == Choice) {
      std::string selection = lowercase (p);
      const char* const * choices = arg->defaults.choices.list;
      for (int i = 0; choices[i]; ++i) {
        if (selection == choices[i]) {
          return i;
        }
      }
      std::string msg = std::string ("unexpected value supplied for ");
      if (opt) msg += std::string ("option \"") + opt->id;
      else msg += std::string ("argument \"") + arg->id;
      msg += std::string ("\" (valid choices are: ") + choices[0];
      for (int i = 1; choices[i]; ++i) {
        msg += ", ";
        msg += choices[i];
      }
      throw Exception (msg + ")");
    }
    assert (0);
    return (0);
  }



  App::ParsedArgument::operator float () const 
  { 
    const float retval = to<float> (p);
    const float min = arg->defaults.i.min;
    const float max = arg->defaults.i.max;
    if (retval < min || retval > max) {
      std::string msg ("value supplied for ");
      if (opt) msg += std::string ("option \"") + opt->id;
      else msg += std::string ("argument \"") + arg->id;
      msg += "\" is out of bounds (valid range: " + str(min) + " to " + str(max) + ", value supplied: " + str(retval) + ")";
      throw Exception (msg);
    }

    return retval;
  }




  App::ParsedArgument::operator double () const 
  { 
    const double retval = to<double> (p);
    const double min = arg->defaults.i.min;
    const double max = arg->defaults.i.max;
    if (retval < min || retval > max) {
      std::string msg ("value supplied for ");
      if (opt) msg += std::string ("option \"") + opt->id;
      else msg += std::string ("argument \"") + arg->id;
      msg += "\" is out of bounds (valid range: " + str(min) + " to " + str(max) + ", value supplied: " + str(retval) + ")";
      throw Exception (msg);
    }

    return retval;
  }

}


