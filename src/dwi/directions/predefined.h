/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#ifndef __dwi_directions_predefined_h__
#define __dwi_directions_predefined_h__


#include "types.h"


namespace MR {
  namespace DWI {
    namespace Directions {

      extern const default_type electrostatic_repulsion_60_data[];
      extern const default_type electrostatic_repulsion_300_data[];
      extern const default_type electrostatic_repulsion_5000_data[];
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
      inline Eigen::MatrixXd electrostatic_repulsion_5000 () { return copy (electrostatic_repulsion_5000_data, 5000); }

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

