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
#include "debug.h"

#define HELP_WIDTH  80

#define HELP_PURPOSE_INDENT 0, 4
#define HELP_ARG_INDENT 8, 20
#define HELP_OPTION_INDENT 2, 20

namespace MR
{
  namespace App
  {

    namespace
    {

      std::string paragraph (
          const std::string& header,
          const std::string& text,
          size_t header_indent,
          size_t indent)
      {
        std::string out, line = std::string (header_indent, ' ') + header + " ";
        if (line.size() < indent)
          line.resize (indent, ' ');

        std::vector<std::string> paragraphs = split (text, "\n");

        for (size_t n = 0; n < paragraphs.size(); ++n) {
          size_t i = 0;
          std::vector<std::string> words = split (paragraphs[n]);
          while (i < words.size()) {
            do {
              line += " " + words[i++];
              if (i >= words.size())
                break;
            }
            while (line.size() + 1 + words[i].size() < HELP_WIDTH);
            out += line + "\n";
            line = std::string (indent, ' ');
          }
        }
        return out;
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
        case ArgFile:
          return ("file");
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
        default:
          return ("undefined");
      }
    }




    std::string Description::syntax () const
    {
      std::string s;
      for (size_t i = 0; i < size(); ++i) 
        s += paragraph ("", (*this)[i], HELP_PURPOSE_INDENT) + "\n";
      return s;
    }

    std::string Argument::syntax () const
    {
      return paragraph (id, desc, HELP_ARG_INDENT);
    }

    std::string ArgumentList::syntax () const
    {
      std::string s;
      for (size_t i = 0; i < size(); ++i)
        s += (*this)[i].syntax();
      return s + "\n";
    }

    std::string Option::syntax () const
    {
      std::string opt ("-");
      opt += id;
      for (size_t i = 0; i < size(); ++i)
        opt += std::string (" ") + (*this)[i].id;
      return paragraph (opt, desc, HELP_OPTION_INDENT);
    }

    std::string OptionGroup::syntax () const
    {
      std::string s = std::string ( name ? name : "OPTIONS" ) + ":\n";
      for (size_t i = 0; i < size(); ++i)
        s += (*this) [i].syntax();
      return s + "\n";
    }

    std::string OptionList::syntax () const
    {
      std::string s;
      for (size_t i = 0; i < size(); ++i)
        s += (*this) [i].syntax();
      return s;
    }








    std::string Argument::usage () const
    {
      std::ostringstream stream;
      stream << "ARGUMENT " << id << " "
        << (flags & Optional ? '1' : '0') << " "
        << (flags & AllowMultiple ? '1' : '0') << " ";

      switch (type) {
        case Integer:
          stream << "INT " << defaults.i.min << " " << defaults.i.max << " " << defaults.i.def;
          break;
        case Float:
          stream << "FLOAT " << defaults.f.min << " " << defaults.f.max << " " << defaults.f.def;
          break;
        case Text:
          stream << "TEXT";
          if (defaults.text)
            stream << " " << defaults.text;
          break;
        case ArgFile:
          stream << "FILE";
          break;
        case Choice:
          stream << "CHOICE";
          for (const char* const* p = defaults.choices.list; *p; ++p)
            stream << " " << *p;
          stream << " " << defaults.choices.def;
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


  }
}


