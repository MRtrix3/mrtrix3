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

#ifndef __app_h__
#define __app_h__

#ifdef MRTRIX_AS_R_LIBRARY

#define MRTRIX_APPLICATION \
extern "C" void R_main (int* cmdline_argc, char** cmdline_argv) { \
    MR::App::AUTHOR = "J-Donald Tournier (d.tournier@brain.org.au)"; \
    MR::App::DESCRIPTION.clear(); \
    MR::App::ARGUMENTS.clear(); \
    MR::App::OPTIONS.clear(); \
    try { usage(); MR::App::init (*cmdline_argc, cmdline_argv); MR::App::parse (); run (); } \
    catch (MR::Exception& E) { E.display(); return; } \
    catch (int retval) { return; } \
} \
extern "C" void R_usage (char** output) { \
    MR::App::DESCRIPTION.clear(); \
    MR::App::ARGUMENTS.clear(); \
    MR::App::OPTIONS.clear(); \
    usage(); \
    std::string s = MR::App::full_usage(); \
    *output = new char [s.size()+1]; \
    strncpy(*output, s.c_str(), s.size()+1); \
}

#else

#define MRTRIX_APPLICATION int main (int cmdline_argc, char** cmdline_argv) { \
    try { MR::App::init (cmdline_argc, cmdline_argv); usage (); MR::App::parse (); run (); } \
    catch (MR::Exception& E) { E.display(); return 1; } \
    catch (int retval) { return retval; } } 

#endif




#include "mrtrix.h"
#include "args.h"
#include <string.h>




extern void usage ();
extern void run ();

namespace MR
{


  namespace App
  {
    extern Description DESCRIPTION;
    extern ArgumentList ARGUMENTS;
    extern OptionList OPTIONS;
    extern bool REQUIRES_AT_LEAST_ONE_ARGUMENT;
    extern OptionGroup __standard_options;
    extern const char* AUTHOR;
    extern const char* COPYRIGHT;
    extern size_t VERSION[3];
    extern int log_level;
    extern std::string NAME;
    extern bool overwrite_files;

    extern int argc;
    extern char** argv;

    //! \addtogroup CmdParse
    // @{

    //! initialise MRtrix and parse command-line arguments
    /*! this function must be called from within main(), immediately after the
     * argument and options have been specified, and before any further
     * processing takes place. */
    void init (int argc, char** argv);
    void parse ();
    const Option* match_option (const char* stub);
    std::string full_usage ();

    class ParsedArgument;

    class Options
    {
      public:
        class Opt
        {
          public:
            const ParsedArgument operator[] (size_t num) const;

          private:
            Opt (const Option* parent, const char* const* arguments) :
              opt (parent), args (arguments) { }

            const Option* opt;
            const char* const* args;
            friend class ParsedArgument;
            friend class Options;
        };


        size_t size () const {
          return args.size();
        }

        const Opt operator[] (size_t num) const {
          assert (num < args.size());
          return Opt (opt, args[num]);
        }


      private:
        Options () { }
        const Option* opt;
        std::vector<const char* const*> args;
        friend const Options get_options (const std::string& name);
    };





    class ParsedArgument
    {
      public:
        operator std::string () const {
          return p;
        }
        operator int () const;
        operator ssize_t () const {
          return operator int ();
        }
        operator size_t () const {
          return operator int ();
        }
        operator float () const;
        operator double () const;

        operator std::vector<int> () const {
          assert (arg->type == IntSeq);
          try {
            return parse_ints (p);
          }
          catch (Exception& e) {
            error (e);
          }
          return std::vector<int>();
        }
        operator std::vector<float> () const {
          assert (arg->type == FloatSeq);
          try {
            return parse_floats (p);
          }
          catch (Exception& e) {
            error (e);
          }
          return std::vector<float>();
        }

        const char* c_str () const {
          return p;
        }

      private:
        const Option* opt;
        const Argument* arg;
        const char* p;

        ParsedArgument (const Option* option, const Argument* argument, const char* text) :
          opt (option), arg (argument), p (text) {
          assert (text);
        }

        void error (Exception& e) const {
          std::string msg ("error parsing token \"");
          msg += p;
          if (opt) msg += std::string ("\" for option \"") + opt->id + "\"";
          else msg += std::string ("\" for argument \"") + arg->id + "\"";
          throw Exception (e, msg);
        }

        friend class Options;
        friend class Options::Opt;
        friend void  MR::App::init (int argc, char** argv);
        friend void  MR::App::parse ();
        friend void  MR::App::sort_arguments (int argc, const char* const* argv);
    };




    class ParsedOption
    {
      public:
        ParsedOption (const Option* option, const char* const* arguments) :
          opt (option), args (arguments) { }

        //! reference to the corresponding Option entry in the OPTIONS section
        const Option* opt;
        //! pointer into \c argv corresponding to the option's first argument
        const char* const* args;

        //! check whether this option matches the name supplied
        bool operator== (const char* match) const {
          std::string name = lowercase (match);
          return name == opt->id;
        }

    };


    //! the list of arguments parsed from the command-line
    extern std::vector<ParsedArgument> argument;
    //! the list of options parsed from the command-line
    extern std::vector<ParsedOption> option;

    //! return all command-line options matching \c name
    /*! This returns a vector of vectors, where each top-level entry
     * corresponds to a distinct instance of the option, and each entry within
     * a top-level entry corresponds to a argument supplied to that option.
     *
     * Individual options can be retrieved easily using implicit type-casting.
     * Any relevant range checks are performed at this point, based on the
     * original App::Option specification. For example:
     * \code
     * Options opt = get_options ("myopt");
     * if (opt.size()) {
     *    std::string arg1 = opt[0][0];
     *    int arg2 = opt[0][1];
     *    float arg3 = opt[0][2];
     *    std::vector<int> arg4 = opt[0][3];
     * }
     * \endcode */
    const Options get_options (const std::string& name);



    //! convenience function provided mostly to ease writing Exception strings
    inline std::string operator+ (const char* left, const App::ParsedArgument& right)
    {
      std::string retval (left);
      retval += std::string (right);
      return retval;
    }

    inline const App::ParsedArgument App::Options::Opt::operator[] (size_t num) const
    {
      assert (num < opt->size());
      return ParsedArgument (opt, & (*opt) [num], args[num]);
    }


    inline std::ostream& operator<< (std::ostream& stream, const App::ParsedArgument& arg)
    {
      stream << std::string (arg);
      return stream;
    }

  }

  //! @}
}

#endif
