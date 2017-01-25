/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __image_transform_h__
#define __image_transform_h__

#include "types.h"

namespace MR
{
  class Transform { MEMALIGN(Transform)
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

