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

#ifndef __image_data_common_h__
#define __image_data_common_h__

#include "image/header.h"
#include "dataset/stride.h"

namespace MR
{
  namespace Image
  {
    //! \cond skip
    
    template <class ArrayType> class Voxel;

    // \endcond


    //! Base class for DataArray object
    /*! This class provides functions commonly used by DataArray objects. It
     * should not be used directly. */
    template <typename T> class DataCommon
    {
      public:
        //! construct a DataCommon object to access the data in the Image::Header \p parent
        DataCommon (const Header& parent) :
          H (parent) {
        }

        const Header& header () const {
          return H;
        }
        DataType datatype () const {
          return H.datatype();
        }
        const Math::Matrix<float>& transform () const {
          return H.transform();
        }

        ssize_t stride (size_t axis) const {
          return H.stride (axis);
        }
        size_t  ndim () const {
          return H.ndim();
        }
        ssize_t dim (size_t axis) const {
          return H.dim (axis);
        }
        float   vox (size_t axis) const {
          return H.vox (axis);
        }
        const std::string& name () const {
          return H.name();
        }

      protected:
        const Header&   H; 
    };




  }
}

#endif




