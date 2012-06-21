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


#ifndef __math_hemisphere_predefined_dirs_h__
#define __math_hemisphere_predefined_dirs_h__


#include "math/matrix.h"


namespace MR {
namespace Math {
namespace Hemisphere {



extern const float directions_60_data[];
extern const float directions_129_data[];
extern const float directions_300_data[];
extern const float directions_469_data[];
extern const float directions_1281_data[];



template <typename T> Math::Matrix<T>& directions_60 (Math::Matrix<T>& dirs)
{
  dirs = Math::Matrix<float> (const_cast<float*> (directions_60_data), 60, 2);
  return dirs;
}


// 3rd-order tessellation of an octahedron
template <typename T> Math::Matrix<T>& directions_129 (Math::Matrix<T>& dirs)
{
  dirs = Math::Matrix<float> (const_cast<float*> (directions_129_data), 129, 2);
  return dirs;
}


template <typename T> Math::Matrix<T>& directions_300 (Math::Matrix<T>& dirs)
{
  dirs = Math::Matrix<float> (const_cast<float*> (directions_300_data), 300, 2);
  return dirs;
}


// 3rd-order tessellation of an icosahedron
template <typename T> Math::Matrix<T>& directions_321 (Math::Matrix<T>& dirs)
{
  dirs = Math::Matrix<float> (const_cast<float*> (directions_321_data), 321, 2);
  return dirs;
}


// 4th-order tessellation of a tetrahedron
template <typename T> Math::Matrix<T>& directions_469 (Math::Matrix<T>& dirs)
{
  dirs = Math::Matrix<float> (const_cast<float*> (directions_469_data), 469, 2);
  return dirs;
}


// 4th-order tessellation of an octahedron
template <typename T> Math::Matrix<T>& directions_513 (Math::Matrix<T>& dirs)
{
  dirs = Math::Matrix<float> (const_cast<float*> (directions_513_data), 513, 2);
  return dirs;
}


template <typename T> Math::Matrix<T>& directions_1281 (Math::Matrix<T>& dirs)
{
  dirs = Math::Matrix<float> (const_cast<float*> (directions_1281_data), 1281, 2);
  return dirs;
}




}
}
}

#endif

