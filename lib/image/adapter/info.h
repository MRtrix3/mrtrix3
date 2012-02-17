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

#ifndef __image_adapter_info_h__
#define __image_adapter_info_h__

#include "debug.h"
#include "types.h"
#include "datatype.h"
#include "math/matrix.h"

namespace MR
{
  namespace Image
  {
    namespace Adapter {

      template <class Set> class Info 
      {
        public:

          Info (Set& parent_image) :
            parent (parent_image) { }

          const std::string& name () const {
            return parent.name();
          }

          DataType datatype () const {
            return parent.datatype();
          }

          size_t ndim () const {
            return parent.ndim();
          }

          int dim (size_t axis) const {
            return parent.dim (axis);
          }

          float vox (size_t axis) const {
            return parent.vox (axis);
          }

          ssize_t stride (size_t axis) const {
            return parent.stride (axis);
          }

          const Math::Matrix<float>& transform () const {
            return parent.transform();
          }

        protected:
          Set& parent;
      };

    }

  }
  //! @}
}

#endif



