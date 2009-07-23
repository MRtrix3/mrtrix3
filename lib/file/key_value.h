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

#ifndef __file_key_value_h__
#define __file_key_value_h__

#include <fstream>
#include "mrtrix.h"

namespace MR {
  namespace File {

    class KeyValue {
      public:
        KeyValue () { }
        KeyValue (const std::string& file, const char* first_line = NULL) { open (file, first_line); }

        void  open (const std::string& file, const char* first_line = NULL);
        bool  next ();
        void  close () { in.close(); }

        const std::string& key () const throw ()   { return (K); }
        const std::string& value () const throw () { return (V); }
        const std::string& name () const throw ()  { return (filename); }

      protected:
        std::string K, V, filename;
        std::ifstream in;
    };

  }
}

#endif

