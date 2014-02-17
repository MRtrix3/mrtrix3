/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __image_nav_h__
#define __image_nav_h__

#include "point.h"


namespace MR
{
namespace Image
{
namespace Nav
{


// Functions for easy navigation of image data


template <class Set, class Nav>
inline void set_pos (Set& data, const Nav& pos)
{
    for (size_t axis = 0; axis != data.ndim(); ++axis)
      data[axis] = pos[axis];
}

template <class Set, class Nav>
inline void set_pos (Set& data, const Nav& pos, const size_t from_axis, const size_t to_axis)
{
    for (size_t axis = from_axis; axis != to_axis; ++axis)
      data[axis] = pos[axis];
}

template <class Set, class Point_type>
inline void set_pos (Set& data, const Point<Point_type>& pos)
{
    for (size_t axis = 0; axis != 3; ++axis)
      data[axis] = pos[axis];
}


template <class Set, class Nav>
inline void get_pos (const Set& data, Nav& pos)
{
    for (size_t axis = 0; axis != data.ndim(); ++axis)
      pos[axis] = data[axis];
}

template <class Set, class Point_type>
inline void get_pos (Set& data, Point<Point_type>& pos)
{
    for (size_t axis = 0; axis != 3; ++axis)
      pos[axis] = data[axis];
}


template <class Set, class Nav>
inline void step_pos (Set& data, const Nav& step)
{
    for (size_t axis = 0; axis != data.ndim(); ++axis)
      data[axis] += step[axis];
}


template <class Set, class Point_type>
inline void step_pos (Set& data, const Point<Point_type>& step)
{
    for (size_t axis = 0; axis != 3; ++axis)
      data[axis] += step[axis];
}



template <class Set, class Nav>
inline typename Set::value_type get_value_at_pos (Set& data, const Nav& pos)
{
    for (size_t axis = 0; axis != data.ndim(); ++axis)
      data[axis] = pos[axis];
    return data.value();
}

template <class Set, class Point_type>
inline typename Set::value_type get_value_at_pos (Set& data, const Point<Point_type>& pos)
{
    for (size_t axis = 0; axis != 3; ++axis)
      data[axis] = pos[axis];
    return data.value();
}


template <class Set, class Nav>
inline void set_value_at_pos (Set& data, const Nav& pos, const typename Set::value_type value)
{
    for (size_t axis = 0; axis != data.ndim(); ++axis)
      data[axis] = pos[axis];
    data.value() = value;
}

template <class Set, class Point_type>
inline void set_value_at_pos (Set& data, const Point<Point_type>& pos, const typename Set::value_type value)
{
    for (size_t axis = 0; axis != 3; ++axis)
      data[axis] = pos[axis];
    data.value() = value;
}



template <class Set, class Nav>
inline bool within_bounds (const Set& data, const Nav& pos)
{
    for (size_t axis = 0; axis != data.ndim(); ++axis)
      if (pos[axis] < 0 || pos[axis] >= data.dim (axis))
        return false;
    return true;
}


template <class Set, class Point_type>
inline bool within_bounds (const Set& data, const Point<Point_type>& pos)
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
}

#endif

