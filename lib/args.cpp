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

      void paragraph (
          const std::string& header,
          const std::string& text,
          size_t header_indent,
          size_t indent)
      {
        std::string out = std::string (header_indent, ' ') + header + " ";
        if (out.size() < indent)
          out.resize (indent, ' ');

        std::vector<std::string> paragraphs = split (text, "\n");

        for (size_t n = 0; n < paragraphs.size(); ++n) {
          size_t i = 0;
          std::vector<std::string> words = split (paragraphs[n]);
          while (i < words.size()) {
            do {
              out += " " + words[i++];
              if (i >= words.size())
                break;
            }
            while (out.size() + 1 + words[i].size() < HELP_WIDTH);
            std::cerr << out << "\n";
            out = std::string (indent, ' ');
          }
        }
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




    void Description::print () const
    {
      for (size_t i = 0; i < size(); ++i) {
        paragraph ("", (*this)[i], HELP_PURPOSE_INDENT);
        std::cerr << "\n";
      }
    }

    void Argument::print () const
    {
      paragraph (id, desc, HELP_ARG_INDENT);
    }

    void ArgumentList::print () const
    {
      for (size_t i = 0; i < size(); ++i)
        (*this)[i].print();
      std::cerr << "\n";
    }

    void Option::print () const
    {
      std::string opt ("-");
      opt += id;
      for (size_t i = 0; i < size(); ++i)
        opt += std::string (" ") + (*this)[i].id;
      paragraph (opt, desc, HELP_OPTION_INDENT);
    }

    void OptionGroup::print () const
    {
      std::cerr << ( name ? name : "OPTIONS" ) << ":\n";
      for (size_t i = 0; i < size(); ++i)
        (*this) [i].print();
      std::cerr << "\n";
    }

    void OptionList::print () const
    {
      for (size_t i = 0; i < size(); ++i)
        (*this) [i].print();
    }








    void Argument::print_usage () const
    {
      std::cout << "ARGUMENT " << id << " "
        << (flags & Optional ? '1' : '0') << " "
        << (flags & AllowMultiple ? '1' : '0') << " ";

      switch (type) {
        case Integer:
          std::cout << "INT " << defaults.i.min << " " << defaults.i.max << " " << defaults.i.def;
          break;
        case Float:
          std::cout << "FLOAT " << defaults.f.min << " " << defaults.f.max << " " << defaults.f.def;
          break;
        case Text:
          std::cout << "TEXT";
          if (defaults.text)
            std::cout << " " << defaults.text;
          break;
        case ArgFile:
          std::cout << "FILE";
          break;
        case Choice:
          std::cout << "CHOICE";
          for (const char** p = defaults.choices.list; *p; ++p)
            std::cout << " " << *p;
          std::cout << " " << defaults.choices.def;
          break;
        case ImageIn:
          std::cout << "IMAGEIN";
          break;
        case ImageOut:
          std::cout << "IMAGEOUT";
          break;
        case IntSeq:
          std::cout << "ISEQ";
          break;
        case FloatSeq:
          std::cout << "FSEQ";
          break;
        default:
          assert (0);
      }
      std::cout << "\n";
      if (desc)
        std::cout << desc << "\n";
    }




    void Option::print_usage () const
    {
      std::cout << "OPTION " << id << " "
        << (flags & Optional ? '1' : '0') << " "
        << (flags & AllowMultiple ? '1' : '0') << "\n";

      if (desc)
        std::cout << desc << "\n";

      for (size_t i = 0; i < size(); ++i)
        (*this)[i].print_usage();
    }


  }
}


