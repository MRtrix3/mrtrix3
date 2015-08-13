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

#include "gui/mrview/tool/connectome/file_data_vector.h"

#include <limits>

#include "file/path.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        FileDataVector::FileDataVector () :
            Math::Vector<float>(),
            min (NAN),
            mean (NAN),
            max (NAN) { }

        FileDataVector::FileDataVector (const FileDataVector& V) :
            Math::Vector<float> (V),
            name (V.name),
            min (V.min),
            mean (V.mean),
            max (V.max) { }

        FileDataVector::FileDataVector (FileDataVector&& V) :
            Math::Vector<float> (std::move (V)),
            name (V.name),
            min (V.min),
            mean (V.mean),
            max (V.max)
        {
          V.name.clear();
          V.min = V.mean = V.max = NAN;
        }

        FileDataVector::FileDataVector (size_t nelements) :
            Math::Vector<float> (nelements),
            min (NAN),
            mean (NAN),
            max (NAN) { }

        FileDataVector::FileDataVector (const std::string& file) :
            Math::Vector<float> (file),
            name (Path::basename (file).c_str()),
            min (NAN),
            mean (NAN),
            max (NAN)
        {
          calc_stats();
        }




        FileDataVector& FileDataVector::operator= (const FileDataVector& that)
        {
          Math::Vector<float>::operator= (that);
          name = that.name;
          min = that.min;
          mean = that.mean;
          max = that.max;
          return *this;
        }
        FileDataVector& FileDataVector::operator= (FileDataVector&& that)
        {
          Math::Vector<float>::operator= (std::move (that));
          name = that.name;
          min = that.min;
          mean = that.mean;
          max = that.max;
          that.name.clear();
          that.min = that.mean = that.max = NAN;
          return *this;
        }




        FileDataVector& FileDataVector::load (const std::string& filename) {
          Math::Vector<float>::load (filename);
          name = Path::basename (filename).c_str();
          calc_stats();
          return *this;
        }

        FileDataVector& FileDataVector::clear ()
        {
          Math::Vector<float>::clear();
          name.clear();
          min = mean = max = NAN;
          return *this;
        }

        void FileDataVector::calc_stats()
        {
          min = std::numeric_limits<float>::max();
          mean = 0.0f;
          max = -std::numeric_limits<float>::max();
          for (size_t i = 0; i != size(); ++i) {
            min = std::min (min, operator[] (i));
            mean += operator[] (i);
            max = std::max (max, operator[] (i));
          }
          mean /= float(size());
        }



      }
    }
  }
}





