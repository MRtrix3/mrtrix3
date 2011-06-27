/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 22/10/09.

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

#ifndef __dataset_subset_h__
#define __dataset_subset_h__

#include "math/matrix.h"
#include "dataset/value.h"
#include "dataset/position.h"

namespace MR
{
  namespace DataSet
  {

    template <class Set>
    class Subset
    {
      private:
        class Axis
        {
          public:
            size_t dim, from;
        };

      public:
        typedef typename Set::value_type value_type;

        Subset (Set& original, const size_t* from, const size_t* dimensions, const std::string& description = "") :
          D (original),
          axes (D.ndim()),
          descriptor (description.empty() ? D.name() + " [subset]" : description),
          transform_matrix (D.transform()) {
          for (size_t n = 0; n < ndim(); ++n) {
            assert (ssize_t (from[n] + dimensions[n]) <= D.dim (n));
            axes[n].from = from[n];
            axes[n].dim = dimensions[n];
          }

          for (size_t j = 0; j < 3; ++j)
            for (size_t i = 0; i < 3; ++i)
              transform_matrix (i,3) += axes[j].from * vox (j) * transform_matrix (i,j);
        }

        Subset (Set& original, size_t NDIM, const size_t* from, const size_t* dimensions, const std::string& description = "") :
          D (original),
          axes (NDIM),
          descriptor (description.empty() ? D.name() + " [subset]" : description),
          transform_matrix (D.transform()) {
          for (size_t n = 0; n < ndim(); ++n) {
            assert (ssize_t (from[n] + dimensions[n]) <= D.dim (n));
            axes[n].from = from[n];
            axes[n].dim = dimensions[n];
          }

          for (size_t j = 0; j < 3; ++j)
            for (size_t i = 0; i < 3; ++i)
              transform_matrix (i,3) += axes[j].from * vox (j) * transform_matrix (i,j);
        }

        const std::string& name () const {
          return (descriptor);
        }
        size_t  ndim () const {
          return (axes.size());
        }
        int     dim (size_t axis) const {
          return (axes[axis].dim);
        }
        ssize_t stride (size_t axis) const {
          return (D.stride (axis));
        }

        float   vox (size_t axis) const {
          return (D.vox (axis));
        }

        const Math::Matrix<float>& transform () const {
          return (transform_matrix);
        }

        void    reset () {
          for (size_t n = 0; n < ndim(); ++n) set_pos (n, 0);
        }

        Value<Subset<Set> > value () {
          return (Value<Subset<Set> > (*this));
        }
        Position<Subset<Set> > operator[] (size_t axis) {
          return (Position<Subset<Set> > (*this, axis));
        }

      private:
        Set& D;
        std::vector<Axis> axes;
        std::string descriptor;
        Math::Matrix<float> transform_matrix;

        value_type get_value () const {
          return (D.value());
        }
        void set_value (value_type val) {
          D.value() = val;
        }
        ssize_t get_pos (size_t axis) const {
          return (D[axis]-axes[axis].from);
        }
        void set_pos (size_t axis, ssize_t position) const {
          D[axis] = position + axes[axis].from;
        }
        void move_pos (size_t axis, ssize_t increment) const {
          D[axis] += increment;
        }

        friend class Value<Subset<Set> >;
        friend class Position<Subset<Set> >;
    };

  }
}

#endif


