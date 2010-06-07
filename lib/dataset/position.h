/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 07/01/10.

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

#ifndef __dataset_position_h__
#define __dataset_position_h__

namespace MR {
  namespace DataSet {

    //! \addtogroup DataSet
    // @{

    template <class Set> class Position 
    {
      public:
        Position (Set& parent, size_t corresponding_axis) : S (parent), axis (corresponding_axis) { }
        operator ssize_t () const          { return (S.get_pos (axis)); }
        ssize_t operator++ ()              { ssize_t p = S.get_pos(axis); S.move_pos (axis,1); return (p); }
        ssize_t operator-- ()              { ssize_t p = S.get_pos(axis); S.move_pos (axis,-1); return (p); }
        ssize_t operator++ (int notused)   { S.move_pos (axis,1); return (S.get_pos (axis)); }
        ssize_t operator-- (int notused)   { S.move_pos (axis,-1); return (S.get_pos (axis)); }
        ssize_t operator+= (ssize_t increment) { S.move_pos (axis, increment); return (S.get_pos(axis)); }
        ssize_t operator-= (ssize_t increment) { S.move_pos (axis, -increment); return (S.get_pos(axis)); }
        ssize_t operator= (ssize_t position)   { S.set_pos (axis, position); return (position); }
        ssize_t operator= (const Position& position)   { S.set_pos (axis, size_t(position)); return (size_t(position)); }
      private:
        Set& S;
        size_t axis;
    };

    //! @}
  }
}

#endif

