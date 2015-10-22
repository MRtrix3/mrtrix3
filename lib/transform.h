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

#include "header.h"
#include "types.h"

namespace MR
{
  class Transform
  {
    public:

      //! An object for transforming between voxel, scanner and image coordinate spaces
      Transform (const Header& header) :
          voxelsize (header.spacing(0), header.spacing(1), header.spacing(2)),
          voxel2scanner (header.transform() * voxelsize),
          scanner2voxel (voxel2scanner.inverse()),
          image2scanner (header.transform()),
          scanner2image (image2scanner.inverse()),
          bounds { header.size(0) - 0.5, header.size(1) - 0.5, header.size(2) - 0.5 },
          out_of_bounds (true) { }

      Transform (const Transform&) = default;
      Transform (Transform&&) = default;
      Transform& operator= (const Transform&) = default;
      Transform& operator= (Transform&&) = default;

      //! test whether current position is within bounds.
      /*! \return true if the current position is out of bounds, false otherwise */
      bool operator! () const { return out_of_bounds; }

      template <typename ValueType> 
        static inline ValueType default_out_of_bounds_value () { 
          return std::numeric_limits<ValueType>::quiet_NaN(); 
        }

      static inline transform_type get_default (const Header& header) {
          transform_type M;
          M.setIdentity();
          M.translation() = Eigen::Vector3d (
              -0.5 * (header.size (0)-1) * header.spacing (0), 
              -0.5 * (header.size (1)-1) * header.spacing (1), 
              -0.5 * (header.size (2)-1) * header.spacing (2)
              );
          return M;
        }

      bool is_out_of_bounds (const Eigen::Vector3d& pos) const {
        if (pos[0] <= -0.5 || pos[0] >= bounds[0] ||
            pos[1] <= -0.5 || pos[1] >= bounds[1] ||
            pos[2] <= -0.5 || pos[2] >= bounds[2]) {
          return true;
        }
        return false;
      }

      const Eigen::DiagonalMatrix<default_type, 3> voxelsize;
      const transform_type voxel2scanner, scanner2voxel, image2scanner, scanner2image;

    protected:
      default_type  bounds[3];
      bool   out_of_bounds;

      template <class VectorType>
        Eigen::Vector3d set_to_nearest (const VectorType& pos) {
          out_of_bounds = is_out_of_bounds (pos);
          if (out_of_bounds)
            return Eigen::Vector3d (default_out_of_bounds_value<default_type>(), default_out_of_bounds_value<default_type>(), default_out_of_bounds_value<default_type>());
          else
            return Eigen::Vector3d (pos[0]-std::floor (pos[0]), pos[1]-std::floor (pos[1]), pos[2]-std::floor (pos[2]));
        }

  };


}

#endif

