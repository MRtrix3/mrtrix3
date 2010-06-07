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

#include "args.h"
#include "app.h"

namespace MR {

  const Argument Argument::End;
  const Option   Option::End;

  const char* argument_type_description (ArgType type)
  {
    switch (type) {
      case Integer:  return ("integer");
      case Float:    return ("float");
      case Text:     return ("string");
      case ArgFile:  return ("file");
      case ImageIn:  return ("image in");
      case ImageOut: return ("image out");
      case Choice:   return ("choice");
      case IntSeq:   return ("int seq");
      case FloatSeq: return ("float seq");
      default:       return ("undefined");
    }
  }


  ArgBase::ArgBase (const Argument& arg, const char* string)
  {
    argtype = arg.type;
    switch (argtype) {
      case Integer: 
        data.i = to<int> (string);
        if (data.i < arg.extra_info.i.min || data.i > arg.extra_info.i.max) 
          throw Exception ("value supplied for integer argument \"" + std::string (arg.sname) + "\" is out of bounds");
        break;
      case Float:
        data.f = to<float> (string);
        if (data.f < arg.extra_info.f.min || data.f > arg.extra_info.f.max) 
          throw Exception ("value supplied for floating-point argument \"" + std::string (arg.sname) + "\" is out of bounds");
        break;
      case Text:
      case ArgFile:
      case IntSeq:
      case FloatSeq:
      case ImageIn:
      case ImageOut:
        data.string = string;
        break;
      case Choice:
        data.i = -1;
        for (size_t n = 0; arg.extra_info.choice[n]; n++) {
          if (lowercase (string) == arg.extra_info.choice[n]) {
            data.i = n;
            break;
          }
        }
        if (data.i < 0) 
          throw Exception ("invalid selection supplied \"" + std::string (string) + "\" for argument \"" + arg.sname + "\"");
        break;
      default: throw Exception ("unkown argument type for argument \"" + std::string (arg.sname) + "\"");
    }
  }




  std::ostream& operator<< (std::ostream& stream, const ArgBase& arg)
  {
    switch (arg.type()) {
      case Integer:  stream << "integer: " << arg.get_int(); break;
      case Float:    stream << "float: " << arg.get_float(); break;
      case Text:     stream << "string: \"" << arg.get_string() << "\""; break;
      case ArgFile:  stream << "file: \"" << arg.get_string() << "\""; break;
      case ImageIn:  stream << "image in: \"" << arg.get_string() << "\""; break;
      case ImageOut: stream << "image out: \"" << arg.get_string() << "\""; break;
      case Choice:   stream << "choice: " << arg.get_int(); break;
      case IntSeq:   stream << "int seq: " << arg.get_string(); break;
      case FloatSeq: stream << "float seq: " << arg.get_string(); break;
      default:       stream << "undefined"; break;
    }
    return (stream);
  }



  std::ostream& operator<< (std::ostream& stream, const OptBase& opt)
  {
    stream << "-" << opt.name << " " << static_cast<std::vector<ArgBase> > (opt);
    return (stream);
  }








  std::ostream& operator<< (std::ostream& stream, const Argument& arg)
  {
    stream << arg.sname << ": " << arg.lname 
      << " (" << argument_type_description (arg.type); 
    switch (arg.type) {
      case Integer:
        if (arg.extra_info.i.def != INT_MAX) stream << ", default=" << arg.extra_info.i.def;
        stream << ", range: " << arg.extra_info.i.min << ":" << arg.extra_info.i.max;
        break;
      case Float:
        if (!isnan (arg.extra_info.f.def)) stream << ", default=" << arg.extra_info.f.def;
        stream << ", range: " << arg.extra_info.f.min << ":" << arg.extra_info.f.max;
        break;
      case Choice:
        {
          const char** p = arg.extra_info.choice;
          stream << " from " << *p;
          while (*(++p)) stream << "|" << *p; 
        }
        break;
      default:
        break;
    }
    stream << ") [" << ( arg.mandatory ? "mandatory" : "optional" ) << ","
      << ( arg.allow_multiple ? "multiple" : "single" ) << "]\n    " << arg.desc;
    return (stream);
  }







  std::ostream& operator<< (std::ostream& stream, const Option& opt)
  {
    stream << opt.sname << ": " << opt.lname << " [" 
      << ( opt.mandatory ? "mandatory" : "optional" ) << ","
      << ( opt.allow_multiple ? "multiple" : "single" ) << "]\n  "
      << opt.desc << "\n\n";
    for (size_t n = 0; n < opt.size(); n++) 
      stream << "[" << n << "] " << opt[n] << "\n\n";
    return (stream);
  }

}


