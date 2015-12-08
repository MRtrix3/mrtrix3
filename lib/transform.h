/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 24/05/09.

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

#ifndef __image_transform_h__
#define __image_transform_h__

#include "types.h"

namespace MR
{
  class Transform
  {
    public:

      //! An object for transforming between voxel, scanner and image coordinate spaces
      template <class HeaderType>
        Transform (const HeaderType& header) :
          voxelsize (header.spacing(0), header.spacing(1), header.spacing(2)),
          voxel2scanner (header.transform() * voxelsize),
          scanner2voxel (voxel2scanner.inverse()),
          image2scanner (header.transform()),
          scanner2image (image2scanner.inverse()) { }


      Transform (const Transform&) = default;
      Transform (Transform&&) = default;
      Transform& operator= (const Transform&) = default;
      Transform& operator= (Transform&&) = default;

      const Eigen::DiagonalMatrix<default_type, 3> voxelsize;
      const transform_type voxel2scanner, scanner2voxel, image2scanner, scanner2image;


      template <class HeaderType>
        static inline transform_type get_default (const HeaderType& header) {
          transform_type M;
          M.setIdentity();
          M.translation() = Eigen::Vector3d (
              -0.5 * (header.size (0)-1) * header.spacing (0),
              -0.5 * (header.size (1)-1) * header.spacing (1),
              -0.5 * (header.size (2)-1) * header.spacing (2)
              );
          return M;
        }

  };


}

#endif

