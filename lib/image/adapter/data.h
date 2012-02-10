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

#ifndef __image_adapter_data_h__
#define __image_adapter_data_h__

#include "image/adapter/info.h"

namespace MR
{
  namespace Image
  {
    namespace Adapter {

      template <class Set> class Data : public Info<Set>
      {
        public:

          Data (Set& parent_image) :
            Info<Set> (parent_image) { }

        typedef typename Set::value_type value_type;
        typedef typename Image::Voxel<Data<Set> > voxel_type;


        value_type get_value (size_t offset) const {
          return parent.get_value();
        }
        void set_value (size_t offset, value_type val) {
          parent.set_value (offset, val);
        }

        using Info<Set>::parent;

      };

    }

  }
  //! @}
}

#endif




