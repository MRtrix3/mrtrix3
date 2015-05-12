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


#ifndef __dwi_directions_predefined_h__
#define __dwi_directions_predefined_h__


#include "math/matrix.h"


namespace MR {
  namespace DWI {
    namespace Directions {

      extern const default_type electrostatic_repulsion_60_data[];
      extern const default_type electrostatic_repulsion_300_data[];
      extern const default_type tesselation_129_data[];
      extern const default_type tesselation_321_data[];
      extern const default_type tesselation_469_data[];
      extern const default_type tesselation_513_data[];
      extern const default_type tesselation_1281_data[];

      namespace {
        inline Eigen::MatrixXd copy (const default_type* p, ssize_t rows) {
          Eigen::MatrixXd d (rows, 2);
          for (ssize_t i = 0; i < rows; ++i) {
            d(i,0) = *p++;
            d(i,1) = *p++;
          }
          return d;
        }
      }



      inline Eigen::MatrixXd electrostatic_repulsion_60 () { return copy (electrostatic_repulsion_60_data, 60); }
      inline Eigen::MatrixXd electrostatic_repulsion_300 () { return copy (electrostatic_repulsion_300_data, 300); }

      //! 3rd-order tessellation of an octahedron
      inline Eigen::MatrixXd tesselation_129 () { return copy (tesselation_129_data, 129); }

      //! 3rd-order tessellation of an icosahedron
      inline Eigen::MatrixXd tesselation_321 () { return copy (tesselation_321_data, 321); }

      //! 4th-order tessellation of a tetrahedron
      inline Eigen::MatrixXd tesselation_469 () { return copy (tesselation_469_data, 469); }

      //! 4th-order tessellation of an octahedron
      inline Eigen::MatrixXd tesselation_513 () { return copy (tesselation_513_data, 513); }

      //! 4th-order tessellation of an icosahedron
      inline Eigen::MatrixXd tesselation_1281 () { return copy (tesselation_1281_data, 1281); }

    }
  }
}

#endif

