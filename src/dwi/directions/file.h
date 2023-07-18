/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __dwi_directions_load_h__
#define __dwi_directions_load_h__

#include "math/sphere.h"

namespace MR {
  namespace DWI {
    namespace Directions {

      Eigen::MatrixXd load_spherical (const std::string& filename);
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

