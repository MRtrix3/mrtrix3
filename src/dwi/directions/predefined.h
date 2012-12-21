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

      extern const float electrostatic_repulsion_60_data[];
      extern const float electrostatic_repulsion_300_data[];
      extern const float tesselation_129_data[];
      extern const float tesselation_321_data[];
      extern const float tesselation_469_data[];
      extern const float tesselation_513_data[];
      extern const float tesselation_1281_data[];



      template <typename T> 
        inline Math::Matrix<T>& electrostatic_repulsion_60 (Math::Matrix<T>& dirs)
        {
          dirs = Math::Matrix<T> (const_cast<float*> (electrostatic_repulsion_60_data), 60, 2);
          return dirs;
        }


      template <typename T> 
        inline Math::Matrix<T>& electrostatic_repulsion_300 (Math::Matrix<T>& dirs)
        {
          dirs = Math::Matrix<T> (const_cast<float*> (electrostatic_repulsion_300_data), 300, 2);
          return dirs;
        }



      //! 3rd-order tessellation of an octahedron
      template <typename T> 
        inline Math::Matrix<T>& tesselation_129 (Math::Matrix<T>& dirs)
        {
          dirs = Math::Matrix<T> (const_cast<float*> (tesselation_129_data), 129, 2);
          return dirs;
        }



      //! 3rd-order tessellation of an icosahedron
      template <typename T> 
        inline Math::Matrix<T>& tesselation_321 (Math::Matrix<T>& dirs)
        {
          dirs = Math::Matrix<T> (const_cast<float*> (tesselation_321_data), 321, 2);
          return dirs;
        }


      //! 4th-order tessellation of a tetrahedron
      template <typename T> 
        inline Math::Matrix<T>& tesselation_469 (Math::Matrix<T>& dirs)
        {
          dirs = Math::Matrix<T> (const_cast<float*> (tesselation_469_data), 469, 2);
          return dirs;
        }


      //! 4th-order tessellation of an octahedron
      template <typename T> 
        inline Math::Matrix<T>& tesselation_513 (Math::Matrix<T>& dirs)
        {
          dirs = Math::Matrix<T> (const_cast<float*> (tesselation_513_data), 513, 2);
          return dirs;
        }


      //! 4th-order tessellation of an icosahedron
      template <typename T> 
        inline Math::Matrix<T>& tesselation_1281 (Math::Matrix<T>& dirs)
        {
          dirs = Math::Matrix<T> (const_cast<float*> (tesselation_1281_data), 1281, 2);
          return dirs;
        }




    }
  }
}

#endif

