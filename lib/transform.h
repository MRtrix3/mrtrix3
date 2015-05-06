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
        Transform (const HeaderType& info) :
          voxelsize (info.voxsize(0), info.voxsize(1), info.voxsize(2)),
          voxel2scanner (info.transform() * voxelsize),
          scanner2voxel (voxel2scanner.inverse()),
          image2scanner (info.transform()),
          scanner2voxel (image2scanner.inverse()),
          bounds ({ info.size(0) - 0.5, info.size(1) - 0.5, info.size(2) - 0.5 }),
          voxelsize ({ info.voxsize(0), info.voxsize(1), info.voxsize(2) }),
          out_of_bounds (true) { }

      //! test whether current position is within bounds.
      /*! \return true if the current position is out of bounds, false otherwise */
      bool operator! () const { return out_of_bounds; }

      template <typename ValueType> 
        static inline ValueType default_out_of_bounds_value () { 
          return std::numeric_limits<ValueType>::quiet_NaN(); 
        }

      template <class HeaderType>
        static inline transform_type get_default (const HeaderType& header) {
          transform_type M;
          M.setIdentity();
          M.translation() = Vector3 (
              -0.5 * (header.size (0)-1) * header.voxsize (0), 
              -0.5 * (header.size (1)-1) * header.voxsize (1), 
              -0.5 * (header.size (2)-1) * header.voxsize (2)
              );
          return M;
        }

      const Eigen::DiagonalMatrix<default_type, 3> voxelsize;
      const transform_type scanner2voxel, voxel2scanner, scanner2image, image2scanner;

    protected:
      default_type  bounds[3];
      bool   out_of_bounds;

      bool check_bounds (const Vector3& pos) const {
        if (pos[0] <= -0.5 || pos[0] >= bounds[0] ||
            pos[1] <= -0.5 || pos[1] >= bounds[1] ||
            pos[2] <= -0.5 || pos[2] >= bounds[2]) {
          return true;
        }
        return false;
      }

      Vector3 set_to_nearest (const Vector3& pos) {
        out_of_bounds = check_bounds (pos);
        if (out_of_bounds)
          return Vector3 (default_out_of_bounds_value<default_type>(), default_out_of_bounds_value<default_type>(), default_out_of_bounds_value<default_type>());
        else
          return Vector3 (pos[0]-std::floor (pos[0]), pos[1]-std::floor (pos[1]), pos[2]-std::floor (pos[2]));
      }

  };


}

#endif

