/*
   Copyright 2014 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2015.

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

#ifndef __gui_mrview_tool_connectome_filedatavector_h__
#define __gui_mrview_tool_connectome_filedatavector_h__

#include <QString>

#include "math/vector.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

      // Vector that stores the name of the file imported, so it can be displayed in the GUI
      class FileDataVector : public Math::Vector<float>
      {
        public:
          FileDataVector ();
          FileDataVector (const FileDataVector& V);
          FileDataVector (FileDataVector&& V);
          FileDataVector (size_t nelements);
          FileDataVector (const std::string& file);

          FileDataVector& operator= (const FileDataVector&);
          FileDataVector& operator= (FileDataVector&&);

          FileDataVector& load (const std::string&);
          FileDataVector& clear();

          const QString& get_name() const { return name; }
          void set_name (const std::string& s) { name = s.c_str(); }

          float get_min() const { return min; }
          float get_max() const { return max; }

          void calc_minmax();

        private:
          QString name;
          float min, max;

      };

      }
    }
  }
}

#endif




