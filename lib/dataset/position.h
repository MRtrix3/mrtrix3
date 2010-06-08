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
        Position (Set& parent, size_t corresponding_axis) : S (parent), axis (corresponding_axis) { assert (axis < S.ndim()); }
        operator ssize_t () const          { return (S.get_pos (axis)); }
        ssize_t operator++ ()              { ssize_t p = S.get_pos(axis); check (p+1); S.move_pos (axis,1); return (p); }
        ssize_t operator-- ()              { ssize_t p = S.get_pos(axis); check (p-1); S.move_pos (axis,-1); return (p); }
        ssize_t operator++ (int notused)   { check (S.get_pos(axis)+1); S.move_pos (axis,1); return (S.get_pos (axis)); }
        ssize_t operator-- (int notused)   { check (S.get_pos(axis)-1); S.move_pos (axis,-1); return (S.get_pos (axis)); }
        ssize_t operator+= (ssize_t increment) { check (S.get_pos(axis)+increment); S.move_pos (axis, increment); return (S.get_pos(axis)); }
        ssize_t operator-= (ssize_t increment) { check (S.get_pos(axis)-increment); S.move_pos (axis, -increment); return (S.get_pos(axis)); }
        ssize_t operator= (ssize_t position)   { check (position); S.set_pos (axis, position); return (position); }
        ssize_t operator= (const Position& position) { check (ssize_t(position)); S.set_pos (axis, ssize_t(position)); return (ssize_t(position)); }
      private:
        Set& S;
        size_t axis;
        void check (ssize_t new_pos) const { assert (new_pos >= 0); assert (new_pos < S.dim(axis)); }
    };

    //! @}
  }
}

#endif

