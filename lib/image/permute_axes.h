/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 02/11/09.

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

#ifndef __image_permute_axes_h__
#define __image_permute_axes_h__

#include "math/matrix.h"
#include "image/value.h"

namespace MR
{
  namespace Image
  {

    template <class Set> class PermuteAxes
    {
      public:
        typedef typename Set::value_type value_type;

        PermuteAxes (Set& original, const std::vector<int>& axes) :
          D (original), A (axes), S (A.size()) {
          for (size_t i = 0; i < A.size(); ++i)
            S[i] = A[i]<0 ? 0 : D.stride (A[i]);

          for (int i = 0; i < static_cast<int> (D.ndim()); ++i) {
            for (size_t a = 0; a < A.size(); ++a)
              if (A[a] == i)
                goto next_axis;
            if (D.dim (i) != 1)
              throw Exception ("ommitted axis \"" + str (i) + "\" has dimension greater than 1");
next_axis:
            continue;
          }
        }
        const std::string& name () const {
          return (D.name());
        }
        size_t  ndim () const {
          return (A.size());
        }
        int     dim (size_t axis) const {
          return (A[axis] < 0 ? 1 : D.dim (A[axis]));
        }
        float   vox (size_t axis) const {
          return (A[axis] < 0 ? NAN : D.vox (A[axis]));
        }
        const Math::Matrix<float>& transform () const {
          return (D.transform());
        }
        ssize_t stride (size_t axis) const {
          return (A[axis] < 0 ? 0 : D.stride (A[axis]));
        }

        void reset () {
          D.reset();
        }

        Position<PermuteAxes<Set> > operator[] (size_t axis) {
          return (Position<PermuteAxes<Set> > (*this, axis));
        }
        Value<PermuteAxes<Set> > value () {
          return (Value<PermuteAxes<Set> > (*this));
        }

      private:
        Set& D;
        size_t* x;
        const std::vector<int> A;
        std::vector<ssize_t> S;

        value_type get_value () const {
          return (D.value());
        }
        void set_value (value_type val) {
          D.value() = val;
        }
        ssize_t get_pos (size_t axis) const {
          return (A[axis] < 0 ? 0 : D[A[axis]]);
        }
        void set_pos (size_t axis, ssize_t position) const {
          D[A[axis]] = position;
        }
        void move_pos (size_t axis, ssize_t increment) const {
          D[A[axis]] += increment;
        }

        friend class Position<PermuteAxes<Set> >;
        friend class Value<PermuteAxes<Set> >;
    };

  }
}

#endif


