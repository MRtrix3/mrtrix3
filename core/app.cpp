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


#include <unistd.h>
#include <fcntl.h>

#include "app.h"
#include "debug.h"
#include "progressbar.h"
#include "file/path.h"
#include "file/config.h"
#include "signal_handler.h"

#define MRTRIX_HELP_COMMAND "less -X"

#define HELP_WIDTH  80

#define HELP_PURPOSE_INDENT 0, 4
#define HELP_ARG_INDENT 8, 20
#define HELP_OPTION_INDENT 2, 20


namespace MR
{

  namespace App
  {

    Description DESCRIPTION;
    ArgumentList ARGUMENTS;
    OptionList OPTIONS;
    Description REFERENCES;
    bool REQUIRES_AT_LEAST_ONE_ARGUMENT = true;

    OptionGroup __standard_options = OptionGroup ("Standard options")
      + Option ("info", "display information messages.")
      + Option ("quiet", "do not display information messages or progress status. Alternatively, this can be achieved by setting the MRTRIX_QUIET environment variable to a non-empty string.")
      + Option ("debug", "display debugging messages.")
      + Option ("force", "force overwrite of output files. "
          "Caution: Using the same file as input and output might cause unexpected behaviour.")
      + Option ("nthreads", "use this number of threads in multi-threaded applications (set to 0 to disable multi-threading).")
        + Argument ("number").type_integer (0)
      + Option ("help", "display this information page and exit.")
      + Option ("version", "display version information and exit.");

    const char* AUTHOR = nullptr;
    const char* COPYRIGHT =
       "Copyright (c) 2008-2018 the MRtrix3 contributors."
       "\n\n"
       "This Source Code Form is subject to the terms of the Mozilla Public\n"
       "License, v. 2.0. If a copy of the MPL was not distributed with this\n"
       "file, you can obtain one at http://mozilla.org/MPL/2.0/\n"
       "\n"
       "MRtrix3 is distributed in the hope that it will be useful,\n"
       "but WITHOUT ANY WARRANTY; without even the implied warranty\n"
       "of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
       "\n"
       "For more details, see http://www.mrtrix.org/\n";
    const char* SYNOPSIS = nullptr;


    std::string NAME;
    vector<ParsedArgument> argument;
    vector<ParsedOption> option;
    int log_level = getenv("MRTRIX_QUIET") ? 0 : 1;
    bool fail_on_warn = false;
    bool terminal_use_colour = true;

    const char* project_version = nullptr;
    const char* build_date = __DATE__;

    int argc = 0;
    const char* const* argv = nullptr;

    bool overwrite_files = false;
    void (*check_overwrite_files_func) (const std::string& name) = nullptr;

    namespace
    {

      inline void get_matches (vector<const Option*>& candidates, const OptionGroup& group, const std::string& stub)
      {
        for (size_t i = 0; i < group.size(); ++i) {
          if (stub.compare (0, stub.size(), group[i].id, stub.size()) == 0)
            candidates.push_back (&group[i]);
        }
      }


      inline int size (const std::string& text)
      {
        return text.size() - 2*std::count (text.begin(), text.end(), 0x08U);
      }

      inline void resize (std::string& text, size_t new_size, char fill)
      {
        text.resize (text.size() + new_size - size(text), fill);
      }



      std::string paragraph (
          const std::string& header,
          const std::string& text,
          int header_indent,
          int indent)
      {
        std::string out, line = std::string (header_indent, ' ') + header + " ";
        if (size (line) < indent)
          resize (line, indent, ' ');

        vector<std::string> paragraphs = split (text, "\n");

        for (size_t n = 0; n < paragraphs.size(); ++n) {
          size_t i = 0;
          vector<std::string> words = split (paragraphs[n]);
          while (i < words.size()) {
            do {
              line += " " + words[i++];
              if (i >= words.size())
                break;
            }
            while (size (line) + 1 + size (words[i]) < HELP_WIDTH);
            out += line + "\n";
            line = std::string (indent, ' ');
          }
        }
        return out;
      }



      std::string bold (const std::string& text)
      {
        std::string retval (3*text.size(), '\0');
        for (size_t n = 0; n < text.size(); ++n) {
          retval[3*n] = retval[3*n+2] = text[n];
          retval[3*n+1] = 0x08U;
        }
        return retval;
      }


      std::string underline (const std::string& text)
      {
        std::string retval (3*text.size(), '\0');
        for (size_t n = 0; n < text.size(); ++n) {
          retval[3*n] = '_';
          retval[3*n+1] = 0x08U;
          retval[3*n+2] = text[n];
        }
        return retval;
      }

    }





    const char* argtype_description (ArgType type)
    {
      switch (type) {
        case Integer:
          return ("integer");
        case Float:
          return ("float");
        case Text:
          return ("string");
        case ArgFileIn:
          return ("file in");
        case ArgFileOut:
          return ("file out");
        case ArgDirectoryIn:
          return ("directory in");
        case ArgDirectoryOut:
          return ("directory out");
        case ImageIn:
          return ("image in");
        case ImageOut:
          return ("image out");
        case Choice:
          return ("choice");
        case IntSeq:
          return ("int seq");
        case FloatSeq:
          return ("float seq");
        case TracksIn:
          return ("tracks in");
        case TracksOut:
          return ("tracks out");
        default:
          return ("undefined");
      }
    }



    std::string help_head (int format)
    {
      std::string cmd_version = project_version ?
        std::string ("external module, version ") + project_version + "\n\n" :
        std::string ("part of the MRtrix package\n\n");

      if (!format)
        return std::string (NAME) + ": " + cmd_version;

      std::string mrtrix_version_string = std::string("MRtrix ") + mrtrix_version;
      std::string date (build_date);

      std::string topline = mrtrix_version_string +
        std::string (std::max (1, 40-size(mrtrix_version_string)-size(App::NAME)/2), ' ')
        + bold (App::NAME);
      topline += std::string (80-size(topline)-size(date), ' ') + date;

      return topline + "\n\n     " + bold (NAME) + ": " + cmd_version;
    }




    std::string help_synopsis (int format)
    {
      if (!format)
        return SYNOPSIS;
      return bold("SYNOPSIS") + "\n" + paragraph ("", SYNOPSIS, HELP_PURPOSE_INDENT) + "\n";
    }




    std::string help_tail (int format)
    {
      std::string retval;
      if (!format)
        return retval;

      return bold ("AUTHOR") + "\n"
        + paragraph ("", AUTHOR, HELP_PURPOSE_INDENT) + "\n"
        + bold ("COPYRIGHT") + "\n"
        + paragraph ("", COPYRIGHT, HELP_PURPOSE_INDENT) + "\n"
        + [&](){
          if (REFERENCES.size() == 0) return std::string();
          std::string s = bold ("REFERENCES") + "\n";
          for (size_t n = 0; n < REFERENCES.size(); ++n)
            s += paragraph ("", REFERENCES[n], HELP_PURPOSE_INDENT) + "\n";
          return s;
        }();
    }






    std::string Description::syntax (int format) const
    {
      if (!size())
        return std::string();
      std::string s;
      if (format)
        s += bold ("DESCRIPTION") + "\n\n";
      for (size_t i = 0; i < size(); ++i)
        s += paragraph ("", (*this)[i], HELP_PURPOSE_INDENT) + "\n";
      return s;
    }




    std::string usage_syntax (int format)
    {
      std::string s = "USAGE";
      if (format)
        s = bold (s) + "\n\n     ";
      else
        s += ": ";
      s += ( format ? underline (NAME) : NAME ) + " [ options ]";

      for (size_t i = 0; i < ARGUMENTS.size(); ++i) {

        if (ARGUMENTS[i].flags & Optional)
          s += "[";
        s += std::string(" ") + ARGUMENTS[i].id;

        if (ARGUMENTS[i].flags & AllowMultiple) {
          if (! (ARGUMENTS[i].flags & Optional))
            s += std::string(" [ ") + ARGUMENTS[i].id;
          s += " ...";
        }
        if (ARGUMENTS[i].flags & (Optional | AllowMultiple))
          s += " ]";
      }
      return s + "\n\n";
    }




    std::string Argument::syntax (int format) const
    {
      std::string retval = paragraph (( format ? underline (id) : id ), desc, HELP_ARG_INDENT);
      if (format)
        retval += "\n";
      return retval;
    }





    std::string ArgumentList::syntax (int format) const
    {
      std::string s;
      for (size_t i = 0; i < size(); ++i)
        s += (*this)[i].syntax (format);
      return s + "\n";
    }





    std::string Option::syntax (int format) const
    {
      std::string opt ("-");
      opt += id;

      if (format)
        opt = underline (opt);

      for (size_t i = 0; i < size(); ++i)
        opt += std::string (" ") + (*this)[i].id;

      if (format)
        opt = "  " + opt + "\n" + paragraph ("", desc, HELP_PURPOSE_INDENT);
      else
        opt = paragraph (opt, desc, HELP_OPTION_INDENT);
      if (format)
        opt += "\n";
      return opt;
    }




    std::string OptionGroup::header (int format) const
    {
      return format ? bold (name) + "\n\n" : std::string (name) + ":\n";
    }

    std::string OptionGroup::contents (int format) const
    {
      std::string s;
      for (size_t i = 0; i < size(); ++i)
        s += (*this)[i].syntax (format);
      return s;
    }

    std::string OptionGroup::footer (int format)
    {
      return format ? "" : "\n";
    }



    std::string OptionList::syntax (int format) const
    {
      vector<std::string> group_names;
      for (size_t i = 0; i < size(); ++i) {
        if (std::find (group_names.begin(), group_names.end(), (*this)[i].name) == group_names.end())
          group_names.push_back ((*this)[i].name);
      }

      std::string s;
      for (size_t i = 0; i < group_names.size(); ++i) {
        size_t n = i;
        while ((*this)[n].name != group_names[i])
          ++n;
        s += (*this)[n].header (format);
        while (n < size()) {
          if ((*this)[n].name == group_names[i])
            s += (*this)[n].contents (format);
          ++n;
        }
        s += OptionGroup::footer (format);
      }

      return s;
    }








    std::string Argument::usage () const
    {
      std::ostringstream stream;
      stream << "ARGUMENT " << id << " "
        << (flags & Optional ? '1' : '0') << " "
        << (flags & AllowMultiple ? '1' : '0') << " ";

      switch (type) {
        case Undefined:
          assert (0);
          break;
        case Integer:
          stream << "INT " << limits.i.min << " " << limits.i.max;
          break;
        case Float:
          stream << "FLOAT " << limits.f.min << " " << limits.f.max;
          break;
        case Text:
          stream << "TEXT";
          break;
        case ArgFileIn:
          stream << "FILEIN";
          break;
        case ArgFileOut:
          stream << "FILEOUT";
          break;
        case ArgDirectoryIn:
          stream << "DIRIN";
          break;
        case ArgDirectoryOut:
          stream << "DIROUT";
          break;
        case Choice:
          stream << "CHOICE";
          for (const char* const* p = limits.choices; *p; ++p)
            stream << " " << *p;
          break;
        case ImageIn:
          stream << "IMAGEIN";
          break;
        case ImageOut:
          stream << "IMAGEOUT";
          break;
        case IntSeq:
          stream << "ISEQ";
          break;
        case FloatSeq:
          stream << "FSEQ";
          break;
        case TracksIn:
          stream << "TRACKSIN";
          break;
        case TracksOut:
          stream << "TRACKSOUT";
          break;
        default:
          assert (0);
      }
      stream << "\n";
      if (desc.size())
        stream << desc << "\n";

      return stream.str();
    }




    std::string Option::usage () const
    {
      std::ostringstream stream;
      stream << "OPTION " << id << " "
        << (flags & Optional ? '1' : '0') << " "
        << (flags & AllowMultiple ? '1' : '0') << "\n";

      if (desc.size())
        stream << desc << "\n";

      for (size_t i = 0; i < size(); ++i)
        stream << (*this)[i].usage ();

      return stream.str();
    }



    std::string get_help_string (int format)
    {
      return
        help_head (format)
        + help_synopsis (format)
        + usage_syntax (format)
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

      //CONF option: HelpCommand
      //CONF default: less
      //CONF The command to use to display each command's help page (leave
      //CONF empty to send directly to the terminal).
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
        "== " + App::NAME + " " + ( project_version ? project_version : mrtrix_version ) + " ==\n" +
        str(8*sizeof (size_t)) + " bit "
#ifdef NDEBUG
        "release"
#else
        "debug"
#endif
        " version, built " __DATE__
        + ( project_version ? std::string(" against MRtrix ") + mrtrix_version : std::string("") )
        + ", using Eigen " + str(EIGEN_WORLD_VERSION) + "." + str(EIGEN_MAJOR_VERSION) + "." + str(EIGEN_MINOR_VERSION) + "\n"
        "Author(s): " + AUTHOR + "\n" +
        COPYRIGHT + "\n";

      return version;
    }





    std::string full_usage ()
    {
      std::string s;
      s += SYNOPSIS + std::string("\n");

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






    std::string markdown_usage ()
    {
      /*
         help_head (format)
         + help_synopsis (format)
         + usage_syntax (format)
         + ARGUMENTS.syntax (format)
         + DESCRIPTION.syntax (format)
         + OPTIONS.syntax (format)
         + __standard_options.header (format)
         + __standard_options.contents (format)
         + __standard_options.footer (format)
         + help_tail (format);
         */
      std::string s = std::string("## Synopsis\n\n") + SYNOPSIS + "\n\n";

      s += "## Usage\n\n    "
        + std::string(NAME) + " [ options ] ";

      // Syntax line:
      for (size_t i = 0; i < ARGUMENTS.size(); ++i) {

        if (ARGUMENTS[i].flags & Optional)
          s += "[";
        s += std::string(" ") + ARGUMENTS[i].id;

        if (ARGUMENTS[i].flags & AllowMultiple) {
          if (! (ARGUMENTS[i].flags & Optional))
            s += std::string(" [ ") + ARGUMENTS[i].id;
          s += " ...";
        }
        if (ARGUMENTS[i].flags & (Optional | AllowMultiple))
          s += " ]";
      }
      s += "\n\n";

      auto indent_newlines = [](std::string text) {
        size_t index = 0;
        while ((index = text.find("\n", index)) != std::string::npos )
          text.replace (index, 1, "<br>");
        return text;
      };

      // Argument description:
      for (size_t i = 0; i < ARGUMENTS.size(); ++i)
        s += std::string("- *") + ARGUMENTS[i].id + "*: " + indent_newlines (ARGUMENTS[i].desc) + "\n";

      if (DESCRIPTION.size()) {
        s += "\n## Description\n\n";
        for (size_t i = 0; i < DESCRIPTION.size(); ++i)
          s += indent_newlines (DESCRIPTION[i]) + "\n\n";
      }


      vector<std::string> group_names;
      for (size_t i = 0; i < OPTIONS.size(); ++i) {
        if (std::find (group_names.begin(), group_names.end(), OPTIONS[i].name) == group_names.end())
          group_names.push_back (OPTIONS[i].name);
      }

      auto format_option = [&](const Option& opt) {
        std::string f = std::string ("+ **-") + opt.id;
        for (size_t a = 0; a < opt.size(); ++a)
          f += std::string (" ") + opt[a].id;
        f += std::string("**<br>") + indent_newlines (opt.desc) + "\n\n";
        return f;
      };

      s += "\n## Options\n\n";
      for (size_t i = 0; i < group_names.size(); ++i) {
        size_t n = i;
        while (OPTIONS[n].name != group_names[i])
          ++n;
        if (OPTIONS[n].name != std::string("OPTIONS"))
          s += std::string ("#### ") + OPTIONS[n].name + "\n\n";
        while (n < OPTIONS.size()) {
          if (OPTIONS[n].name == group_names[i]) {
            for (size_t o = 0; o < OPTIONS[n].size(); ++o)
              s += format_option (OPTIONS[n][o]);
          }
          ++n;
        }
      }

      s += "#### Standard options\n\n";
      for (size_t i = 0; i < __standard_options.size(); ++i)
        s += format_option (__standard_options[i]);

      if (REFERENCES.size()) {
        s += std::string ("## References\n\n");
        for (size_t i = 0; i < REFERENCES.size(); ++i)
          s += indent_newlines (REFERENCES[i]) + "\n\n";
      }
      s += std::string("---\n\nMRtrix ") + mrtrix_version + ", built " + build_date + "\n\n"
        "\n\n**Author:** " + AUTHOR
        + "\n\n**Copyright:** " + COPYRIGHT + "\n\n";

      return s;
    }




    std::string restructured_text_usage ()
    {
      /*
         help_head (format)
         + help_synopsis (format)
         + usage_syntax (format)
         + ARGUMENTS.syntax (format)
         + DESCRIPTION.syntax (format)
         + OPTIONS.syntax (format)
         + __standard_options.header (format)
         + __standard_options.contents (format)
         + __standard_options.footer (format)
         + help_tail (format);
         */

      std::string s = std::string("Synopsis\n--------\n\n") + SYNOPSIS + "\n\n";

      s += "Usage\n--------\n\n::\n\n    "
        + std::string(NAME) + " [ options ] ";

      // Syntax line:
      for (size_t i = 0; i < ARGUMENTS.size(); ++i) {

        if (ARGUMENTS[i].flags & Optional)
          s += "[";
        s += std::string(" ") + ARGUMENTS[i].id;

        if (ARGUMENTS[i].flags & AllowMultiple) {
          if (! (ARGUMENTS[i].flags & Optional))
            s += std::string(" [ ") + ARGUMENTS[i].id;
          s += " ...";
        }
        if (ARGUMENTS[i].flags & (Optional | AllowMultiple))
          s += " ]";
      }
      s += "\n\n";

      auto indent_newlines = [](std::string text) {
        size_t index = 0;
        while ((index = text.find("\n", index)) != std::string::npos )
          text.replace (index, 1, "");
        return text;
      };

      // Will need more sophisticated escaping of special characters
      //   if they start popping up in argument / option descriptions
      auto escape_special = [] (std::string text) {
        size_t index = 0;
        while ((index = text.find("|", index)) != std::string::npos) {
          text.replace (index, 1, "\\|");
          index += 2;
        }
        return text;
      };

      // Argument description:
      for (size_t i = 0; i < ARGUMENTS.size(); ++i)
        s += std::string("-  *") + ARGUMENTS[i].id + "*: " + escape_special (indent_newlines (ARGUMENTS[i].desc)) + "\n";
      s += "\n";


      if (DESCRIPTION.size()) {
        s += "Description\n-----------\n\n";
        for (size_t i = 0; i < DESCRIPTION.size(); ++i)
          s += indent_newlines (DESCRIPTION[i]) + "\n\n";
      }


      vector<std::string> group_names;
      for (size_t i = 0; i < OPTIONS.size(); ++i) {
        if (std::find (group_names.begin(), group_names.end(), OPTIONS[i].name) == group_names.end())
          group_names.push_back (OPTIONS[i].name);
      }

      auto format_option = [&](const Option& opt) {
        std::string f = std::string ("-  **-") + opt.id;
        for (size_t a = 0; a < opt.size(); ++a)
          f += std::string (" ") + opt[a].id;
        f += std::string("** ") + escape_special (indent_newlines (opt.desc)) + "\n\n";
        return f;
      };

      s += "Options\n-------\n\n";
      for (size_t i = 0; i < group_names.size(); ++i) {
        size_t n = i;
        while (OPTIONS[n].name != group_names[i])
          ++n;
        if (OPTIONS[n].name != std::string("OPTIONS"))
          s += OPTIONS[n].name + std::string("\n") + std::string(std::strlen(OPTIONS[n].name), '^') + "\n\n";
        while (n < OPTIONS.size()) {
          if (OPTIONS[n].name == group_names[i]) {
            for (size_t o = 0; o < OPTIONS[n].size(); ++o)
              s += format_option (OPTIONS[n][o]);
          }
          ++n;
        }
      }

      s += "Standard options\n^^^^^^^^^^^^^^^^\n\n";
      for (size_t i = 0; i < __standard_options.size(); ++i)
        s += format_option (__standard_options[i]);

      if (REFERENCES.size()) {
        s += std::string ("References\n^^^^^^^^^^\n\n");
        for (size_t i = 0; i < REFERENCES.size(); ++i)
          s += indent_newlines (REFERENCES[i]) + "\n\n";
      }
      s += std::string("--------------\n\n") +
        "\n\n**Author:** " + (char*)AUTHOR
        + "\n\n**Copyright:** " + COPYRIGHT + "\n\n";

      return s;
    }






    const Option* match_option (const char* arg)
    {
      if (arg[0] == '-' && arg[1] && !isdigit (arg[1]) && arg[1] != '.') {
        while (*arg == '-') arg++;
        vector<const Option*> candidates;
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
          root += std::string ("\", \"-") + candidates[i]->id + "\"";

        throw Exception (root);
      }

      return nullptr;
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
          argument.push_back (ParsedArgument (nullptr, nullptr, argv[n]));
      }
    }




    void parse_standard_options()
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
    }



    void verify_usage ()
    {
      if (!AUTHOR)
        throw Exception ("No author specified for command " + std::string(NAME));
      if (!SYNOPSIS)
        throw Exception ("No synopsis specified for command " + std::string(NAME));
    }



    void parse_special_options ()
    {
      if (argc != 2) return;
      if (strcmp (argv[1], "__print_full_usage__") == 0) {
        print (full_usage ());
        throw 0;
      }
      if (strcmp (argv[1], "__print_usage_markdown__") == 0) {
        print (markdown_usage ());
        throw 0;
      }
      if (strcmp (argv[1], "__print_usage_rst__") == 0) {
        print (restructured_text_usage ());
        throw 0;
      }
      if (strcmp (argv[1], "__print_synopsis__") == 0) {
        print (SYNOPSIS);
        throw 0;
      }
    }



    void parse ()
    {
      argument.clear();
      option.clear();

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
        throw Exception ("Expected at least " + str (num_args_required)
            + " arguments (" + str (argument.size()) + " supplied)");

      if (num_optional_arguments == 0 && num_args_required != argument.size()) {
        Exception e ("Expected exactly " + str (num_args_required)
            + " arguments (" + str (argument.size()) + " supplied)");
        std::string s = "Usage: " + NAME;
        for (const auto& a : ARGUMENTS)
          s += " " + std::string(a.id);
        e.push_back (s);
        s = "Yours: " + NAME;
        for (const auto& a : argument)
          s += " " + std::string(a);
        e.push_back (s);
        if (argument.size() > num_args_required) {
          vector<std::string> potential_options;
          for (const auto& a : argument) {
            for (const auto& og : OPTIONS) {
              for (const auto& o : og) {
                if (std::string(a) == std::string(o.id))
                  potential_options.push_back ("'-" + a + "'");
              }
            }
          }
          if (potential_options.size())
            e.push_back ("(Did you mean " + join(potential_options, " or ") + "?)");
        }
        throw e;
      }

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

      parse_standard_options();

      File::Config::init ();

      //CONF option: FailOnWarn
      //CONF default: 0 (false)
      //CONF A boolean value specifying whether MRtrix applications should
      //CONF abort as soon as any (otherwise non-fatal) warning is issued.
      fail_on_warn = File::Config::get_bool ("FailOnWarn", false);

      //CONF option: TerminalColor
      //CONF default: 1 (true)
      //CONF A boolean value to indicate whether colours should be used in the terminal.
      terminal_use_colour = File::Config::get_bool ("TerminalColor", terminal_use_colour);

      // check for the existence of all specified input files (including optional ones that have been provided)
      // if necessary, also check for pre-existence of any output files with known paths
      //   (if the output is e.g. given as a prefix, the argument should be flagged as type_text())
      for (const auto& i : argument) {
        const std::string text = std::string (i);
        if (i.arg->type == ArgFileIn || i.arg->type == TracksIn) {
          if (!Path::exists (text))
            throw Exception ("required input file \"" + text + "\" not found");
          if (!Path::is_file (text))
            throw Exception ("required input \"" + text + "\" is not a file");
        }
        if (i.arg->type == ArgDirectoryIn) {
          if (!Path::exists (text))
            throw Exception ("required input directory \"" + text + "\" not found");
          if (!Path::is_dir (text))
            throw Exception ("required input \"" + text + "\" is not a directory");
        }
        if (i.arg->type == ArgFileOut || i.arg->type == TracksOut) {
          if (text.find_last_of (PATH_SEPARATOR) == text.size() - std::string(PATH_SEPARATOR).size())
            throw Exception ("output path \"" + std::string(i) + "\" is not a valid file path (ends with \'" PATH_SEPARATOR "\")");
          check_overwrite (text);
        }
        if (i.arg->type == ArgDirectoryOut)
          check_overwrite (text);
        if (i.arg->type == TracksIn && !Path::has_suffix (text, ".tck"))
          throw Exception ("input file \"" + text + "\" is not a valid track file");
        if (i.arg->type == TracksOut && !Path::has_suffix (text, ".tck"))
          throw Exception ("output track file \"" + text + "\" must use the .tck suffix");
        if (i.arg->type == ArgDirectoryOut) {
          const std::string base_name = Path::basename (text);
          if (base_name.find (".") != std::string::npos)
            throw Exception ("output path \"" + text + "\" is not a valid directory path (contains a \'.\')");
        }
      }
      for (const auto& i : option) {
        for (size_t j = 0; j != i.opt->size(); ++j) {
          const Argument& arg = i.opt->operator [](j);
          const std::string text = std::string (i.args[j]);
          if (arg.type == ArgFileIn || arg.type == TracksIn) {
            if (!Path::exists (text))
              throw Exception ("input file \"" + text + "\" for option \"-" + std::string(i.opt->id) + "\" not found");
            if (!Path::is_file (text))
              throw Exception ("input \"" + text + "\" for option \"-" + std::string(i.opt->id) + "\" is not a file");
          }
          if (arg.type == ArgDirectoryIn) {
            if (!Path::exists (text))
              throw Exception ("input directory \"" + text + "\" for option \"-" + std::string(i.opt->id) + "\" not found");
            if (!Path::is_dir (text))
              throw Exception ("input \"" + text + "\" for option \"-" + std::string(i.opt->id) + "\" is not a directory");
          }
          if (arg.type == ArgFileOut || arg.type == TracksOut) {
            if (text.find_last_of (PATH_SEPARATOR) == text.size() - std::string (PATH_SEPARATOR).size())
              throw Exception ("output path \"" + text + "\" for option \"-" + std::string(i.opt->id) + "\" is not a valid file path (ends with \'" PATH_SEPARATOR "\")");
            check_overwrite (text);
          }
          if (arg.type == ArgDirectoryOut)
            check_overwrite (text);
          if (arg.type == TracksIn && !Path::has_suffix (text, ".tck"))
            throw Exception ("input file \"" + text + "\" for option \"-" + std::string(i.opt->id) + "\" is not a valid track file");
          if (arg.type == TracksOut && !Path::has_suffix (text, ".tck"))
            throw Exception ("output track file \"" + text + "\" for option \"-" + std::string(i.opt->id) + "\" must use the .tck suffix");
          if (arg.type == ArgDirectoryOut) {
            const std::string base_name = Path::basename (text);
            if (base_name.find (".") != std::string::npos)
              throw Exception ("output path \"" + text + "\" for option \"-" + std::string(i.opt->id) + "\" is not a valid directory path (contains a \'.\')");
          }
        }
      }

      SignalHandler::init();
    }




    void init (int cmdline_argc, const char* const* cmdline_argv)
    {
#ifdef MRTRIX_WINDOWS
      // force stderr to be unbuffered, and stdout to be line-buffered:
      setvbuf (stderr, nullptr, _IONBF, 0);
      setvbuf (stdout, nullptr, _IOLBF, 0);
#endif

      terminal_use_colour = !ProgressBar::set_update_method();

      argc = cmdline_argc;
      argv = cmdline_argv;

      NAME = Path::basename (argv[0]);
#ifdef MRTRIX_WINDOWS
      if (Path::has_suffix (NAME, ".exe"))
        NAME.erase (NAME.size()-4);
#endif

      srand (time (nullptr));
    }








    const vector<ParsedOption> get_options (const std::string& name)
    {
      vector<ParsedOption> matches;
      for (size_t i = 0; i < option.size(); ++i) {
        assert (option[i].opt);
        if (option[i].opt->is (name))
          matches.push_back ({ option[i].opt, option[i].args });
      }
      return matches;
    }






    int64_t App::ParsedArgument::as_int () const
    {
      if (arg->type == Integer) {

        // Check to see if there are any alpha characters in here
        // - If a single character at the end, use as integer multiplier
        //   - Unless there's a dot point before the multiplier; in which case,
        //     parse the number as a float, multiply, then cast to integer
        // - If a single 'e' or 'E' in the middle, parse as float and convert to integer
        size_t alpha_count = 0;
        bool alpha_is_last = false;
        bool contains_dotpoint = false;
        char alpha_char = 0;
        for (const char* c = p; *c; ++c) {
          if (std::isalpha (*c)) {
            ++alpha_count;
            alpha_is_last = true;
            alpha_char = *c;
          } else {
            alpha_is_last = false;
          }
          if (*c == '.')
            contains_dotpoint = true;
        }
        if (alpha_count > 1)
          throw Exception ("error converting string " + str(p) + " to integer: too many letters");
        int64_t retval = 0;
        if (alpha_count) {
          if (alpha_is_last) {
            std::string num (p);
            const char postfix = num.back();
            num.pop_back();
            int64_t multiplier = 1.0;
            switch (postfix) {
              case 'k': case 'K': multiplier = 1000; break;
              case 'm': case 'M': multiplier = 1000000; break;
              case 'b': case 'B': multiplier = 1000000000; break;
              case 't': case 'T': multiplier = 1000000000000; break;
              default: throw Exception ("error converting string " + str(p) + " to integer: unexpected postfix \'" + postfix + "\'");
            }
            if (contains_dotpoint) {
              const default_type prefix = to<default_type> (num);
              retval = std::round (prefix * default_type(multiplier));
            } else {
              retval = to<int64_t> (num) * multiplier;
            }
          } else if (alpha_char == 'e' || alpha_char == 'E') {
            const default_type as_float = to<default_type> (p);
            retval = std::round (as_float);
          } else {
            throw Exception ("error converting string " + str(p) + " to integer: unexpected character");
          }
        } else {
          retval = to<int64_t> (p);
        }

        const int64_t min = arg->limits.i.min;
        const int64_t max = arg->limits.i.max;
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
        const char* const* choices = arg->limits.choices;
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



    default_type App::ParsedArgument::as_float () const
    {
      assert (arg->type == Float);
      const default_type retval = to<default_type> (p);
      const default_type min = arg->limits.f.min;
      const default_type max = arg->limits.f.max;
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


