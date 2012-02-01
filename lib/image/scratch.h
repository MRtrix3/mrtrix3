/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 23/05/09.

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

#ifndef __image_scratch_h__
#define __image_scratch_h__

#include "debug.h"
#include "image/info.h"
#include "image/stride.h"
#include "image/adapter/info.h"

namespace MR
{
  namespace Image
  {


    template <typename T> class Scratch : public ConstInfo
    {
      public:
        Scratch (const Info& info) :
          ConstInfo (info),
          data_ (new value_type [Image::voxel_count (*this)]) {
            datatype_ = DataType::from<value_type>();
        }

        Scratch (const Info& info, const std::string& label) :
          ConstInfo (info),
          data_ (new value_type [Image::voxel_count (*this)]) {
            datatype_ = DataType::from<value_type>();
            name_ = label;
        }

        typedef T value_type;
        typedef Image::Voxel<Scratch> voxel_type;

        value_type get_value (size_t index) const {
          return data_[index];
        }

        void set_value (size_t index, value_type val) {
          data_[index] = val;
        }

        friend std::ostream& operator<< (std::ostream& stream, const Scratch& V) {
          stream << "scratch image data \"" << V.name() << "\": " + str (Image::voxel_count (V))
            + " voxels in " + V.datatype().specifier() + " format, stored at address " + str ((void*) V.data_);
          return stream;
        }

      protected:
        Ptr<value_type,true> data_;
    };




  }
}

#endif




