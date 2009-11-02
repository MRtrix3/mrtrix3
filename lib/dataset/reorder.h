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

namespace MR {
  namespace DataSet {

    //! \addtogroup DataSet
    // @{

    template <class Set> class Reorder {
      public:
        typedef typename Set::value_type value_type;

        Reorder (Set& original, const size_t* ordering = NULL, const std::string& description = "") : 
          D (original),
          order (new size_t [ndim()]),
          descriptor (description.empty() ? D.name() + " [reordered]" : description) {
            memcpy (order, ordering ? ordering : D.layout(), ndim()*sizeof(size_t));
        }

        const std::string& name () const { return (descriptor); }
        size_t  ndim () const { return (D.ndim()); }
        int     dim (size_t axis) const { return (D.dim(order[axis])); }
        const size_t* layout () const { return (order); }

        float   vox (size_t axis) const { return (D.vox(order[axis])); }

        const Math::Matrix<float>& transform () const { return (D.transform()); }

        void    reset () { D.reset(); }

        ssize_t pos (size_t axis) const { return (D.pos(order[axis])); }
        void    pos (size_t axis, ssize_t position) const { D.pos(order[axis], position); }
        void    move (size_t axis, ssize_t increment) const { D.move (order[axis], increment); }

        value_type   value () const { return (D.value()); }
        void         value (value_type val) { D.value (val); }

      private:
        Set& D;
        size_t* order;
        std::string descriptor;
    };

    //! @}
  }
}

#endif


