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

#include "args.h"

// TODO: are these really necessary?
#define SET_VERSION(a, b, c) const size_t __command_version[3] = { (a), (b), (c) }
#define SET_VERSION_DEFAULT SET_VERSION (MRTRIX_MAJOR_VERSION, MRTRIX_MINOR_VERSION, MRTRIX_MICRO_VERSION); 
#define SET_AUTHOR(a) const char* __command_author = (a)
#define SET_COPYRIGHT(a) const char* __command_copyright = (a)

#define DESCRIPTION const char* __command_description[]
#define ARGUMENTS   const MR::Argument __command_arguments[]
#define OPTIONS const MR::Option __command_options[]

#define __EXECUTE__(_code_) class MyApp : public MR::App { \
  public: \
  MyApp (int argc, char** argv, const char** cmd_desc, const MR::Argument* cmd_args, const MR::Option* cmd_opts, \
      const size_t* cmd_version, const char* cmd_author, const char* cmd_copyright) : \
  App (argc, argv, cmd_desc, cmd_args, cmd_opts, cmd_version, cmd_author, cmd_copyright) { } \
  void execute (); \
}; \
int main (int argc, char* argv[]) { \
  _code_ \
  return (0); } \
void MyApp::execute ()


#ifdef NDEBUG
# define EXECUTE __EXECUTE__( \
  try { \
    MyApp app (argc, argv, __command_description, __command_arguments, __command_options, __command_version, __command_author, __command_copyright); \
    app.run (); \
  } \
  catch (Exception& E) { E.display(); return (1); } \
  catch (int ret) { return (ret); } )

#else
# define EXECUTE __EXECUTE__( \
  try { \
    MyApp app (argc, argv, __command_description, __command_arguments, __command_options, __command_version, __command_author, __command_copyright); \
    app.run (); \
  } \
  catch (Exception& E) { E.display(); throw; } )
#endif


#define DEFAULT_OPTIONS_OFFSET 65536U


namespace MR {
  
  namespace Viewer { class Window; }


  //! \addtogroup CmdParse 
  // @{

  class App
  {
    public:
      App (int argc, char** argv, const char** cmd_desc, const MR::Argument* cmd_args, const MR::Option* cmd_opts,
          const size_t* cmd_version, const char* cmd_author, const char* cmd_copyright);

      virtual ~App ();

      void   run () { parse_arguments (); execute (); }

      static int log_level;

      static const size_t*      version;
      static const char*        copyright;
      static const char*        author;
      static const std::string& name () { return (application_name); }

    protected:
      class ParsedArgument;

      class Options {
        public:

          class Opt {
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


          size_t size () const { return args.size(); }

          const Opt operator[] (size_t num) const
          {
            assert (num < args.size());
            return Opt (opt, args[num]); 
          }


        private:
          const Option* opt;
          std::vector<const char* const*> args;
          friend class App;
      };

      class ParsedArgument {
        public:
          operator std::string () const { return p; } 
          operator int () const;
          operator size_t () const { return operator int (); }
          operator float () const;
          operator double () const;

          operator std::vector<int> () const
          {
            assert (arg->type == IntSeq);
            try { return parse_ints (p); }
            catch (Exception& e) { error (e); }
            return std::vector<int>();
          }
          operator std::vector<float> () const
          {
            assert (arg->type == FloatSeq);
            try { return parse_floats (p); }
            catch (Exception& e) { error (e); }
            return std::vector<float>();
          }

          const char* c_str () const { return p; } 

        private:
          ParsedArgument (const Option* option, const Argument* argument, const char* text) :
            opt (option), arg (argument), p(text) 
        { assert (text); }

          const Option* opt;
          const Argument* arg;
          const char* p;

          void error (Exception& e) const 
          {
            std::string msg ("error parsing token \"");
            msg += p;
            if (opt) msg += std::string("\" for option \"") + opt->id + "\"";
            else msg += std::string("\" for argument \"") + arg->id + "\"";
            throw Exception (e, msg);
          }

          friend class App;
          friend class Options;
          friend class Options::Opt;
      };

      class ParsedOption {
        public:
          ParsedOption (const Option* option, const char* const* arguments) : 
            opt (option), args (arguments) { assert (opt); }

          //! pointer to the corresponding Option entry in the OPTIONS section
          const Option* opt;
          //! pointer into \c argv corresponding to the option's first argument
          const char* const* args;

          //! check whether this option matches the name supplied
          bool operator== (const char* match) const 
          {
            std::string name = lowercase (match);
            return name == opt->id;
          }

      };



      //! the list of arguments parsed from the command-line
      std::vector<ParsedArgument> argument;
      //! the list of options parsed from the command-line
      std::vector<ParsedOption> option;

      virtual void execute () = 0;

      //! find all matching options and provide a list of their arguments
      Options get_options (const std::string& name)
      {
        Options matches;
        for (std::vector<ParsedOption>::const_iterator opt = option.begin(); 
            opt != option.end(); ++opt) {
          assert (opt->opt);
          if (name == opt->opt->id) {
            matches.opt = opt->opt;
            matches.args.push_back (opt->args);
          }
        }
        return matches;
      }

      //! this should not be called directly
      void parse_arguments ();

    private:

      void   print_help () const;
      void   print_full_usage () const;
      const Option* match_option (const char* stub) const;
      void   sort_arguments (int argc, char** argv);

      static std::string     application_name;
      static const char**    command_description;
      static const Argument* command_arguments;
      static const Option*   command_options;
      static const Option    default_options[];

      friend std::string operator+ (const char* left, const ParsedArgument& right);
      friend std::ostream& operator<< (std::ostream& stream, const App::ParsedArgument& arg);
  };


  //! convenience function provided mostly to ease writing Exception strings
  inline std::string operator+ (const char* left, const App::ParsedArgument& right) 
  {
    std::string retval (left);
    retval += std::string (right);
    return retval;
  }

  inline const App::ParsedArgument App::Options::Opt::operator[] (size_t num) const 
  {
    assert (num < opt->args.size());
    return ParsedArgument (opt, &opt->args[num], args[num]); 
  }


  inline std::ostream& operator<< (std::ostream& stream, const App::ParsedArgument& arg)
  {
    stream << std::string (arg);
    return stream;
  }
  //! @}


  void cmdline_print (const std::string& msg);
  void cmdline_error (const std::string& msg);
  void cmdline_info  (const std::string& msg);
  void cmdline_debug (const std::string& msg);



}

#endif
