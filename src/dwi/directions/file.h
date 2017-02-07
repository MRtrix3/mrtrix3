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


#ifndef __dwi_directions_load_h__
#define __dwi_directions_load_h__

#include "math/sphere.h"

namespace MR {
  namespace DWI {
    namespace Directions {

      Eigen::MatrixXd load_cartesian (const std::string& filename);

      template <class MatrixType>
        inline void save_cartesian (const MatrixType& directions, const std::string& filename) 
        {
          if (directions.cols() == 2) 
            save_matrix (Math::Sphere::spherical2cartesian (directions), filename);
          else 
            save_matrix (directions, filename);
        }

      template <class MatrixType>
        inline void save_spherical (const MatrixType& directions, const std::string& filename) 
        {
          if (directions.cols() == 3) 
            save_matrix (Math::Sphere::cartesian2spherical (directions), filename);
          else 
            save_matrix (directions, filename);
        }

      template <class MatrixType>
        inline void save (const MatrixType& directions, const std::string& filename, bool cartesian) 
        {
          if (cartesian) 
            save_cartesian (directions, filename);
          else 
            save_spherical (directions, filename);
        }


    }
  }
}

#endif

