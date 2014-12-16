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
#include <gsl/gsl_errno.h>

#include "app.h"
#include "debug.h"
#include "progressbar.h"
#include "file/path.h"
#include "file/config.h"
#include "version.h"

#define MRTRIX_HELP_COMMAND "less"


namespace MR
{


  void mrtrix_gsl_error_handler (const char* reason, const char* file, int line, int gsl_errno)
  {
    throw Exception (std::string ("GSL error: ") + reason);
  }



  namespace App
  {

    Description DESCRIPTION;
    ArgumentList ARGUMENTS;
    OptionList OPTIONS;
    bool REQUIRES_AT_LEAST_ONE_ARGUMENT = true;

    OptionGroup __standard_options = OptionGroup ("Standard options")
                                     + Option ("info", "display information messages.")
                                     + Option ("quiet", "do not display information messages or progress status.")
                                     + Option ("debug", "display debugging messages.")
                                     + Option ("force", "force overwrite of output files.")
                                     + Option ("nthreads", "use this number of threads in multi-threaded applications")
                                       + Argument ("number").type_integer (0, 1, std::numeric_limits<int>::max())
                                     + Option ("failonwarn", "terminate program if a warning is produced")
                                     + Option ("help", "display this information page and exit.")
                                     + Option ("version", "display version information and exit.");

    const char* AUTHOR = "J-Donald Tournier (d.tournier@brain.org.au)";
    const char* COPYRIGHT =
      "Copyright (C) 2008 Brain Research Institute, Melbourne, Australia.\n"
      "This is free software; see the source for copying conditions.\n"
      "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.";
    const char* REFERENCES = NULL;


    std::string NAME;
    bool overwrite_files = false;
    std::vector<ParsedArgument> argument;
    std::vector<ParsedOption> option;
    int log_level = 1;
    bool fail_on_warn = false;

    int argc = 0;
    char** argv = NULL;




    namespace
    {

      inline void get_matches (std::vector<const Option*>& candidates, const OptionGroup& group, const std::string& stub)
      {
        for (size_t i = 0; i < group.size(); ++i) {
          if (stub.compare (0, stub.size(), group[i].id, stub.size()) == 0)
            candidates.push_back (&group[i]);
        }
      }




      std::string get_help_string (int format) 
      {
        return
          help_head (format)
          + help_syntax (format)
          + ARGUMENTS.syntax (format)
          + DESCRIPTION.syntax (format)
          + OPTIONS.syntax (format)
          + __standard_options.header (format)
          + __standard_options.contents (format)
          + __standard_options.footer (format)
          + help_tail (format);
      }




      void print_help ()
      {
        File::Config::init ();

        const std::string help_display_command = File::Config::get ("HelpCommand", MRTRIX_HELP_COMMAND); 

        if (help_display_command.size()) {
          std::string help_string = get_help_string (1);
          FILE* file = popen (help_display_command.c_str(), "w");
          if (!file) {
            INFO ("error launching help display command \"" + help_display_command + "\": " + strerror (errno));
          }
          else if (fwrite (help_string.c_str(), 1, help_string.size(), file) != help_string.size()) {
            INFO ("error sending help page to display command \"" + help_display_command + "\": " + strerror (errno));
          }

          if (pclose (file) == 0)
            return;

          INFO ("error launching help display command \"" + help_display_command + "\"");
        }

        if (help_display_command.size()) 
          INFO ("displaying help page using fail-safe output:\n");

        print (get_help_string (0));
      }





      std::string version_string ()
      {
        std::string version = 
          "== " + App::NAME + " " + ( project_version ? project_version : MRTRIX_GIT_VERSION ) + " ==\n" +
          str(8*sizeof (size_t)) + " bit " 
#ifdef NDEBUG
          "release"
#else
          "debug"
#endif
          " version, built " __DATE__ + ( project_version ? " against MRtrix " MRTRIX_GIT_VERSION : "" ) + ", using GSL " + gsl_version + "\n"
          "Author(s): " + AUTHOR + "\n" +
          COPYRIGHT + "\n";

        return version;
      }

    }




    std::string full_usage ()
    {
      std::string s;
      for (size_t i = 0; i < DESCRIPTION.size(); ++i)
        s += DESCRIPTION[i] + std::string("\n");

      for (size_t i = 0; i < ARGUMENTS.size(); ++i)
        s += ARGUMENTS[i].usage();

      for (size_t i = 0; i < OPTIONS.size(); ++i)
        for (size_t j = 0; j < OPTIONS[i].size(); ++j)
          s += OPTIONS[i][j].usage();

      for (size_t i = 0; i < __standard_options.size(); ++i)
        s += __standard_options[i].usage ();

      return s;
    }





    const Option* match_option (const char* arg)
    {
      if (arg[0] == '-' && arg[1] && !isdigit (arg[1]) && arg[1] != '.') {
        while (*arg == '-') arg++;
        std::vector<const Option*> candidates;
        std::string root (arg);

        for (size_t i = 0; i < OPTIONS.size(); ++i)
          get_matches (candidates, OPTIONS[i], root);
        get_matches (candidates, __standard_options, root);

        // no matches
        if (candidates.size() == 0)
          throw Exception (std::string ("unknown option \"-") + root + "\"");

        // return match if unique:
        if (candidates.size() == 1)
          return candidates[0];

        // return match if fully specified:
        for (size_t i = 0; i < candidates.size(); ++i)
          if (root == candidates[i]->id)
            return candidates[i];

        // report something useful:
        root = "several matches possible for option \"-" + root + "\": \"-" + candidates[0]->id;

        for (size_t i = 1; i < candidates.size(); ++i)
          root += std::string (", \"-") + candidates[i]->id + "\"";

        throw Exception (root);
      }

      return NULL;
    }




    void sort_arguments (int argc, const char* const* argv)
    {
      for (int n = 1; n < argc; ++n) {
        const Option* opt = match_option (argv[n]);
        if (opt) {
          if (n + int (opt->size()) >= argc)
            throw Exception (std::string ("not enough parameters to option \"-") + opt->id + "\"");

          option.push_back (ParsedOption (opt, argv+n+1));
          n += opt->size();
        }
        else
          argument.push_back (ParsedArgument (NULL, NULL, argv[n]));
      }
    }




    void load_standard_options()
    {
      if (get_options ("info").size()) {
        if (log_level < 2)
          log_level = 2;
      }
      if (get_options ("debug").size())
        log_level = 3;
      if (get_options ("quiet").size())
        log_level = 0;
      if (get_options ("force").size()) {
        WARN ("existing output files will be overwritten");
        overwrite_files = true;
      }
      if (get_options ("failonwarn").size() || File::Config::get_bool ("FailOnWarn", false))
        fail_on_warn = true;
    }




    void parse ()
    {
       argument.clear();
       option.clear();

      if (argc == 2) {
        if (strcmp (argv[1], "__print_full_usage__") == 0) {
          print (full_usage ());
          throw 0;
        }
      }

      sort_arguments (argc, argv);

      if (get_options ("help").size()) {
        print_help();
        throw 0;
      }
      if (get_options ("version").size()) {
        print (version_string());
        throw 0;
      }

      size_t num_args_required = 0, num_command_arguments = 0;
      size_t num_optional_arguments = 0;

      ArgFlags flags = None;
      for (size_t i = 0; i < ARGUMENTS.size(); ++i) {
        ++num_command_arguments;
        if (ARGUMENTS[i].flags) {
          if (flags && flags != ARGUMENTS[i].flags)
            throw Exception ("FIXME: all arguments declared optional() or allow_multiple() should have matching flags in command-line syntax");
          flags = ARGUMENTS[i].flags;
          ++num_optional_arguments;
          if (!(flags & Optional))
            ++num_args_required;
        }
        else 
          ++num_args_required;
      }

      if (!option.size() && !argument.size() && REQUIRES_AT_LEAST_ONE_ARGUMENT) {
        print_help ();
        throw 0;
      }

      if (num_optional_arguments && num_args_required > argument.size())
        throw Exception ("expected at least " + str (num_args_required)
                         + " arguments (" + str (argument.size()) + " supplied)");

      if (num_optional_arguments == 0 && num_args_required != argument.size())
        throw Exception ("expected exactly " + str (num_args_required)
                         + " arguments (" + str (argument.size()) + " supplied)");

      size_t num_extra_arguments = argument.size() - num_args_required;
      size_t num_arg_per_multi = num_optional_arguments ? num_extra_arguments / num_optional_arguments : 0;
      if (num_arg_per_multi*num_optional_arguments != num_extra_arguments)
        throw Exception ("number of optional arguments provided are not equal for all arguments");
      if (!(flags & Optional))
        ++num_arg_per_multi;

      // assign arguments to their corresponding definitions:
      for (size_t n = 0, index = 0, next = 0; n < argument.size(); ++n) {
        if (n == next) {
          if (n) ++index;
          if (ARGUMENTS[index].flags != None)
            next = n + num_arg_per_multi;
          else
            ++next;
        }
        argument[n].arg = &ARGUMENTS[index];
      }

      // check for multiple instances of options:
      for (size_t i = 0; i < OPTIONS.size(); ++i) {
        for (size_t j = 0; j < OPTIONS[i].size(); ++j) {
          size_t count = 0;
          for (size_t k = 0; k < option.size(); ++k)
            if (option[k].opt == &OPTIONS[i][j])
              count++;

          if (count < 1 && ! (OPTIONS[i][j].flags & Optional))
            throw Exception (std::string ("mandatory option \"") + OPTIONS[i][j].id + "\" must be specified");

          if (count > 1 && ! (OPTIONS[i][j].flags & AllowMultiple))
            throw Exception (std::string ("multiple instances of option \"") +  OPTIONS[i][j].id + "\" are not allowed");
        }
      }

      File::Config::init ();

      load_standard_options();

      // check for the existence of all specified input files (including optional ones that have been provided)
      // if necessary, also check for pre-existence of any output files with known paths
      //   (if the output is e.g. given as a prefix, the argument should be flagged as type_text())
      for (std::vector<ParsedArgument>::const_iterator i = argument.begin(); i < argument.end(); i++) {
        if ((i->arg->type == ArgFileIn) && !Path::exists (std::string(*i)))
          throw Exception ("required input file \"" + str(*i) + "\" not found");
        if (!overwrite_files && (i->arg->type == ArgFileOut) && Path::exists (std::string(*i)))
          throw Exception ("output file \"" + std::string(*i) + "\" already exists (use -force option to force overwrite)");
      }
      for (std::vector<ParsedOption>::const_iterator i = option.begin(); i != option.end(); ++i) {
        for (size_t j = 0; j != i->opt->size(); ++j) {
          const Argument& arg = i->opt->operator [](j);
          const char* const name = i->args[j];
          if ((arg.type == ArgFileIn) && !Path::exists (name))
            throw Exception ("input file \"" + str(name) + "\" not found (required for option \"-" + std::string(i->opt->id) + "\")");
          if (!overwrite_files && (arg.type == ArgFileOut) && Path::exists (name))
            throw Exception ("output file \"" + str(name) + "\" already exists (required for option \"-" + std::string(i->opt->id) + "\" - use -force option to force overwrite)");
        }
      }
    }




    void init (int cmdline_argc, char** cmdline_argv)
    {
#ifdef MRTRIX_WINDOWS
      // force stderr to be unbuffered, and stdout to be line-buffered:
      setvbuf (stderr, NULL, _IONBF, 0);
      setvbuf (stdout, NULL, _IOLBF, 0);
#endif

      argc = cmdline_argc;
      argv = cmdline_argv;

      NAME = Path::basename (argv[0]);
#ifdef MRTRIX_WINDOWS
      if (Path::has_suffix (NAME, ".exe"))
        NAME.erase (NAME.size()-4);
#endif

      srand (time (NULL));

      gsl_set_error_handler (&mrtrix_gsl_error_handler);
    }








    const Options get_options (const std::string& name)
    {
      using namespace App;
      Options matches;
      for (size_t i = 0; i < option.size(); ++i) {
        assert (option[i].opt);
        if (option[i].opt->is (name)) {
          matches.opt = option[i].opt;
          matches.args.push_back (option[i].args);
        }
      }
      return matches;
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
          msg += "\" is out of bounds (valid range: " + str (min) + " to " + str (max) + ", value supplied: " + str (retval) + ")";
          throw Exception (msg);
        }
        return retval;
      }

      if (arg->type == Choice) {
        std::string selection = lowercase (p);
        const char* const* choices = arg->defaults.choices.list;
        for (int i = 0; choices[i]; ++i) {
          if (selection == choices[i]) {
            return i;
          }
        }
        std::string msg = std::string ("unexpected value supplied for ");
        if (opt) msg += std::string ("option \"") + opt->id;
        else msg += std::string ("argument \"") + arg->id;
        msg += std::string ("\" (valid choices are: ") + join (choices, ", ") + ")";
        throw Exception (msg);
      }
      assert (0);
      return (0);
    }



    App::ParsedArgument::operator float () const
    {
      const float retval = to<float> (p);
      const float min = arg->defaults.f.min;
      const float max = arg->defaults.f.max;
      if (retval < min || retval > max) {
        std::string msg ("value supplied for ");
        if (opt) msg += std::string ("option \"") + opt->id;
        else msg += std::string ("argument \"") + arg->id;
        msg += "\" is out of bounds (valid range: " + str (min) + " to " + str (max) + ", value supplied: " + str (retval) + ")";
        throw Exception (msg);
      }

      return retval;
    }




    App::ParsedArgument::operator double () const
    {
      const double retval = to<double> (p);
      const double min = arg->defaults.f.min;
      const double max = arg->defaults.f.max;
      if (retval < min || retval > max) {
        std::string msg ("value supplied for ");
        if (opt) msg += std::string ("option \"") + opt->id;
        else msg += std::string ("argument \"") + arg->id;
        msg += "\" is out of bounds (valid range: " + str (min) + " to " + str (max) + ", value supplied: " + str (retval) + ")";
        throw Exception (msg);
      }

      return retval;
    }
  }

}


