/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 2014

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


#ifndef __dwi_directions_load_h__
#define __dwi_directions_load_h__

#include "math/SH.h"

namespace MR {
  namespace DWI {
    namespace Directions {

      Eigen::MatrixXd load_cartesian (const std::string& filename);

      template <class MatrixType>
        inline void save_cartesian (const MatrixType& directions, const std::string& filename) 
        {
          if (directions.cols() == 2) 
            save_matrix (Math::SH::spherical2cartesian (directions), filename);
          else 
            save_matrix (directions, filename);
        }

      template <class MatrixType>
        inline void save_spherical (const MatrixType& directions, const std::string& filename) 
        {
          if (directions.cols() == 3) 
            save_matrix (Math::SH::cartesian2spherical (directions), filename);
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

