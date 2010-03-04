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

#ifndef __args_h__
#define __args_h__

#include "ptr.h"
#include "image/header.h"

#ifdef None
# undef None
#endif

namespace MR {

  namespace Dialog {
    class Window;
    class Argument;
  }

  /*! \defgroup CmdParse Command-Line Parsing
   * \brief Classes and functions to parse command-line arguments and options.
   * */

  //! \addtogroup CmdParse 
  // @{

  typedef enum {
    Undefined,
    Integer,
    Float,
    Text,
    ArgFile,
    Choice,
    ImageIn,
    ImageOut,
    IntSeq,
    FloatSeq
  } ArgType;

  typedef int ArgFlags;
  const ArgFlags None = 0;
  const ArgFlags Optional = 0x1;
  const ArgFlags AllowMultiple = 0x2;


  const char* argument_type_description (ArgType type);


  class Argument;

  class ArgBase {
    public:
      ArgBase () : argtype (Undefined) { data.string = NULL; }
      ArgBase (const Argument& arg, const char* string);

      int         get_int () const     { return (data.i); }
      float       get_float () const   { return (data.f); }
      const char* get_string () const  { return (data.string); }
      const Image::Header get_image (bool readwrite = false) const { 
        assert (type() == ImageIn);
        return (Image::Header::open (data.string, readwrite));
      }
      const Image::Header get_image (const Image::Header& template_header) const { 
        assert (type() == ImageOut); 
        return (Image::Header::create (data.string, template_header)); 
      }

      ArgType type () const { return (argtype); }

      friend class Dialog::Window;
      friend class Dialog::Argument;

    protected:
      ArgType argtype;
      union { int i; float f; const char* string; } data;
  };

  std::ostream& operator<< (std::ostream& stream, const ArgBase& arg);




  class OptBase : public std::vector<ArgBase> {
    public:
      size_t index;

      friend std::ostream& operator<< (std::ostream& stream, const OptBase& opt);
      friend class Dialog::Window;
  };


  typedef std::vector<OptBase> OptionList;


  class Argument {
    public:
      Argument () : type (Undefined) { sname = lname = desc = NULL; } 
      Argument (const char* Short_Name, const char* Long_Name, const char* Description, ArgFlags flags = None) :
        sname (Short_Name), lname (Long_Name), desc (Description), mandatory (!(flags & Optional)), allow_multiple (flags & AllowMultiple) { }

      const char* sname;
      const char* lname;
      const char* desc;
      bool mandatory, allow_multiple;
      ArgType type;

      union {
        struct { int def, min, max; }     i;
        struct { float def, min, max; }   f;
        const char*                       string;
        const char**                      choice;
      } extra_info;

      const Argument& type_integer (int lowest, int highest, int default_value);
      const Argument& type_float   (float lowest, float highest, float default_value);
      const Argument& type_string  (const char* default_value = NULL);
      const Argument& type_file    ();
      const Argument& type_image_in ();
      const Argument& type_image_out ();
      const Argument& type_choice  (const char** choices);
      const Argument& type_sequence_int ();
      const Argument& type_sequence_float ();

      bool is_valid () const { return (sname); }

      static const Argument End;
      friend std::ostream& operator<< (std::ostream& stream, const Argument& arg);
      friend class ArgBase;
  };





  class Option : public std::vector<Argument> {
    public:
      Option () { sname = lname = desc = NULL; } 
      Option (const char* Short_Name, const char* Long_Name, const char* Description, ArgFlags flags = Optional) :
        sname (Short_Name), lname (Long_Name), desc (Description), mandatory (!(flags & Optional)), allow_multiple (flags & AllowMultiple) { }

      const char* sname;
      const char* lname;
      const char* desc;
      bool mandatory, allow_multiple;

      Option& append (const Argument& argument) { std::vector<Argument>::push_back (argument); return (*this); }
      bool    is_valid () const { return (sname); }

      friend std::ostream& operator<< (std::ostream& stream, const Option& opt);
      friend std::ostream& operator<< (std::ostream& stream, const OptBase& opt);

      static const Option End;
  };










  inline const Argument& Argument::type_integer (int lowest, int highest, int default_value)
  {
    type = Integer;
    extra_info.i.def = default_value;
    extra_info.i.min = lowest;
    extra_info.i.max = highest;
    return (*this);
  }

  inline const Argument& Argument::type_float (float lowest, float highest, float default_value)
  {
    type = Float;
    extra_info.f.def = default_value;
    extra_info.f.min = lowest;
    extra_info.f.max = highest;
    return (*this);
  }

  inline const Argument& Argument::type_string (const char* default_value)
  {
    type = Text;
    extra_info.string = default_value;
    return (*this);
  }

  inline const Argument& Argument::type_choice (const char** choices)
  {
    type = Choice;
    extra_info.choice = choices;
    return (*this);
  }

  inline const Argument& Argument::type_file ()
  {
    type = ArgFile;
    extra_info.string = NULL;
    return (*this);
  }

  inline const Argument& Argument::type_image_in ()
  {
    type = ImageIn;
    extra_info.string = NULL;
    return (*this);
  }

  inline const Argument& Argument::type_image_out ()
  {
    type = ImageOut;
    extra_info.string = NULL;
    return (*this);
  }

  inline const Argument& Argument::type_sequence_int ()
  {
    type = IntSeq;
    extra_info.string = NULL;
    return (*this);
  }

  inline const Argument& Argument::type_sequence_float ()
  {
    type = FloatSeq;
    extra_info.string = NULL;
    return (*this);
  }

  //! @}

}

#endif

