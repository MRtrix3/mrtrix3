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
    MyApp app (argc, argv, __command_description, __command_arguments, __command_options, __command_version, __command_author, __command_copyright); \
    app.run ();)
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
      class ParsedOption {
        public:
          ParsedOption (const char* name, const char* const* arguments) : 
            id (name), args (arguments) { }
          const char* id;
          const char* const* args;

          bool operator== (const char* match) const 
          {
            std::string name = lowercase (match);
            return (name == id);
          }

      };

      std::vector<const char*> argument;
      std::vector<ParsedOption> option;

      virtual void execute () = 0;

      typedef std::vector<const char* const*> Options;
      Options get_options (const std::string& name)
      {
        Options matches;
        for (std::vector<ParsedOption>::const_iterator opt = option.begin(); 
            opt != option.end(); ++opt) {
          assert (opt->id);
          if (name == opt->id) 
            matches.push_back (opt->args);
        }
        return (matches);
      }

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

/*
      const char* option_name (size_t num) const 
      { 
        return (num < DEFAULT_OPTIONS_OFFSET ? 
            command_options[num].id : 
            default_options[num - DEFAULT_OPTIONS_OFFSET].id 
            ); 
      }
*/
  };

  //! @}


  void cmdline_print (const std::string& msg);
  void cmdline_error (const std::string& msg);
  void cmdline_info  (const std::string& msg);
  void cmdline_debug (const std::string& msg);

}

#endif
