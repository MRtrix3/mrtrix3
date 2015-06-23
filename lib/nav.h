/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 23/05/09.

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

#ifndef __image_nav_h__
#define __image_nav_h__

#include "types.h"

namespace MR
{
namespace Nav
{


// Functions for easy navigation of image data


template <class ImageType, class Nav>
inline void set_pos (ImageType& data, const Nav& pos)
{
    for (size_t axis = 0; axis != data.ndim(); ++axis)
      data[axis] = pos[axis];
}

template <class ImageType, class Nav>
inline void set_pos (ImageType& data, const Nav& pos, const size_t from_axis, const size_t to_axis)
{
    for (size_t axis = from_axis; axis != to_axis; ++axis)
      data[axis] = pos[axis];
}

template <class ImageType>
inline void set_pos (ImageType& data, const Eigen::Vector3& pos)
{
    for (size_t axis = 0; axis != 3; ++axis)
      data[axis] = pos[axis];
}


template <class ImageType, class Nav>
inline void get_pos (const ImageType& data, Nav& pos)
{
    for (size_t axis = 0; axis != data.ndim(); ++axis)
      pos[axis] = data[axis];
}

template <class ImageType>
inline void get_pos (ImageType& data, Eigen::Vector3& pos)
{
    for (size_t axis = 0; axis != 3; ++axis)
      pos[axis] = data[axis];
}


template <class ImageType, class Nav>
inline void step_pos (ImageType& data, const Nav& step)
{
    for (size_t axis = 0; axis != data.ndim(); ++axis)
      data[axis] += step[axis];
}


template <class ImageType>
inline void step_pos (ImageType& data, const Eigen::Vector3& step)
{
    for (size_t axis = 0; axis != 3; ++axis)
      data[axis] += step[axis];
}



template <class ImageType, class Nav>
inline typename ImageType::value_type get_value_at_pos (ImageType& data, const Nav& pos)
{
    for (size_t axis = 0; axis != data.ndim(); ++axis)
      data[axis] = pos[axis];
    return data.value();
}

template <class ImageType>
inline typename ImageType::value_type get_value_at_pos (ImageType& data, const Eigen::Vector3& pos)
{
    for (size_t axis = 0; axis != 3; ++axis)
      data[axis] = pos[axis];
    return data.value();
}


template <class ImageType, class Nav>
inline void set_value_at_pos (ImageType& data, const Nav& pos, const typename ImageType::value_type value)
{
    for (size_t axis = 0; axis != data.ndim(); ++axis)
      data[axis] = pos[axis];
    data.value() = value;
}

template <class ImageType>
inline void set_value_at_pos (ImageType& data, const Eigen::Vector3& pos, const typename ImageType::value_type value)
{
    for (size_t axis = 0; axis != 3; ++axis)
      data[axis] = pos[axis];
    data.value() = value;
}



template <class ImageType, class Nav>
inline bool within_bounds (const ImageType& data, const Nav& pos)
{
    for (size_t axis = 0; axis != data.ndim(); ++axis)
      if (pos[axis] < 0 || pos[axis] >= data.dim (axis))
        return false;
    return true;
}


template <class ImageType>
inline bool within_bounds (const ImageType& data, const Eigen::Vector3& pos)
{
    for (size_t axis = 0; axis != 3; ++axis)
      if (pos[axis] < 0 || pos[axis] >= data.dim (axis))
        return false;
    return true;
}


template <class Nav>
inline bool within_bounds (const Nav& pos)
{
    for (size_t axis = 0; axis != pos.ndim(); ++axis)
      if (pos[axis] < 0 || pos[axis] >= pos.dim (axis))
        return false;
    return true;
}



}
}

#endif

