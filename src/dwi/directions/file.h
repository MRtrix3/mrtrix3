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
#include "math/matrix.h"

namespace MR {
  namespace DWI {
    namespace Directions {

      template <typename ValueType>
        inline Math::Matrix<ValueType> load_cartesian (const std::string& filename) 
        {
          Math::Matrix<ValueType> directions (filename);
          if (directions.columns() == 2) 
            directions = Math::SH::S2C (directions);
          else {
            if (directions.columns() != 3)
              throw Exception ("unexpected number of columns for directions file \"" + filename + "\"");
            for (size_t n  = 0; n < directions.rows(); ++n) {
              ValueType norm = Math::norm (directions.row(n));
              if (std::abs(ValueType(1.0) - norm) > 1e-4)
                WARN ("directions file \"" + filename + "\" contains non-unit direction vectors");
              directions.row(n) *= norm ? ValueType(1.0)/norm : ValueType(0.0);
            }
          }
          return directions;
        }


      template <typename ValueType>
        inline void save_cartesian (const Math::Matrix<ValueType>& directions, const std::string& filename) 
        {
          if (directions.columns() == 2) {
            Math::Matrix<ValueType> output = Math::SH::S2C (directions);
            output.save (filename);
          }
          else 
            directions.save (filename);
        }

      template <typename ValueType>
        inline void save_spherical (const Math::Matrix<ValueType>& directions, const std::string& filename) 
        {
          if (directions.columns() == 3) {
            Math::Matrix<ValueType> output = Math::SH::C2S (directions);
            output.save (filename);
          }
          else 
            directions.save (filename);
        }

      template <typename ValueType>
        inline void save (const Math::Matrix<ValueType>& directions, const std::string& filename, bool cartesian) 
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

