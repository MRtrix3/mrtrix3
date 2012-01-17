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
#include "image/data_common.h"
#include "image/stride.h"

namespace MR
{
  namespace Image
  {


    //! This class provides access to the voxel intensities of an image.
    /*! This class keeps a reference to an existing Image::Header, and provides
     * access to the corresponding image intensities.
     * \todo Provide specialisations of get/set methods to handle conversions
     * between floating-point and integer types */
    template <typename T> class Scratch : public DataCommon<T>
    {
      using DataCommon<T>::H;

      public:
        //! construct a Scratch object based on the Image::Header \p template
        Scratch (const Header& parent, const std::string& name = "unnamed") :
          DataCommon<T> (parent),
          name_ (name),
          stride_ (Image::Stride::get (H)),
          data (new T [Image::voxel_count (H)]) {
        }

        //! construct a Scratch object based on the Image::Header \p template, with the strides specified
        Scratch (const Header& parent, const Image::Stride::List& strides, const std::string& name = "unnamed") :
          DataCommon<T> (parent),
          name_ (name),
          stride_ (Image::Stride::get_actual (strides, H)),
          data (new T [Image::voxel_count (H)]) {
        }

        typedef T value_type;
        typedef Image::Voxel<Scratch> voxel_type;

        DataType datatype () const {
          return DataType::from<value_type>();
        }

        ssize_t stride (size_t axis) const {
          return stride_[axis];
        }
        const std::string& name () const {
          return name_;
        }

        value_type get (size_t index) const {
          return data[index];
        }

        void set (size_t index, value_type val) const {
          data[index] = val;
        }

        friend std::ostream& operator<< (std::ostream& stream, const Scratch& V) {
          stream << "scratch image data \"" << V.name() << "\": " + str (Image::voxel_count (V))
            + " voxels in " + V.datatype().specifier() + " format, stored at address " + str (size_t (&(*V.data)));
          return stream;
        }

      private:
        const std::string name_;
        const Image::Stride::List stride_;
        value_type* data;

    };




  }
}

#endif




