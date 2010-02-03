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

#ifndef __dataset_reorder_h__
#define __dataset_reorder_h__

#include "math/matrix.h"
#include "dataset/misc.h"
#include "dataset/value.h"
#include "dataset/position.h"

namespace MR {
  namespace DataSet {

    //! \addtogroup DataSet
    // @{

    template <class Set> class Reorder {
      public:
        typedef typename Set::value_type value_type;

        Reorder (Set& original, const std::string& description = "") : 
          D (original),
          S (stride_order (original)),
          descriptor (description.empty() ? D.name() + " [reordered]" : description) { }

        template <class Set2> Reorder (Set& original, const Set2& reference, const std::string& description = "") : 
          D (original),
          S (stride_order (reference)),
          descriptor (description.empty() ? D.name() + " [reordered]" : description) { }

        Reorder (Set& original, const std::vector<size_t>& ordering, const std::string& description = "") : 
          D (original),
          S (ordering),
          descriptor (description.empty() ? D.name() + " [reordered]" : description) { }

        const std::string& name () const { return (descriptor); }
        size_t  ndim () const { return (D.ndim()); }
        int     dim (size_t axis) const { return (D.dim(S[axis])); }
        ssize_t stride (size_t axis) const { return (D.stride (S[axis])); }
        const std::vector<size_t>& order () const { return (S); }

        float   vox (size_t axis) const { return (D.vox(S[axis])); }

        const Math::Matrix<float>& transform () const { return (D.transform()); }

        void    reset () { D.reset(); }

        Position<Reorder<Set> > operator[] (size_t axis) { return (Position<Reorder<Set> > (*this, axis)); }
        Value<Reorder<Set> > value () { return (Value<Reorder<Set> > (*this)); }

      private:
        Set& D;
        std::vector<size_t> S;
        std::string descriptor;

        ssize_t get_pos (size_t axis) const { return (D[S[axis]]); }
        void    set_pos (size_t axis, ssize_t position) const { D[S[axis]] = position; }
        void    move_pos (size_t axis, ssize_t increment) const { D[S[axis]] += increment; }
        value_type get_value () const { return (D.value()); }
        void set_value (value_type val) { D.value() = val; }

        friend class Position<Reorder<Set> >;
        friend class Value<Reorder<Set> >;
    };

    //! @}
  }
}

#endif


