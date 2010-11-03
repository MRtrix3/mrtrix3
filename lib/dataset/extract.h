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

#ifndef __dataset_extract_h__
#define __dataset_extract_h__

#include "math/matrix.h"
#include "dataset/value.h"

namespace MR {
  namespace DataSet {

    //! \addtogroup DataSet
    // @{

    template <class Set> class Extract {
      public:
        typedef typename Set::value_type value_type;

        Extract (Set& original, const std::vector<std::vector<int> >& positions) : 
          D (original), x (new size_t [ndim()]), P (positions), _transform (D.transform()) { 
            reset();

            Math::Vector<float> a (4), b(4);
            a[0] = P[0][0] * vox(0);
            a[1] = P[1][0] * vox(1);
            a[2] = P[2][0] * vox(2);
            a[3] = 1.0;
            Math::mult (b, D.transform(), a);
            _transform.column(3) = b;
          }
        ~Extract () { delete [] x; }
        const std::string& name () const { return (D.name()); }
        size_t  ndim () const { return (D.ndim()); }
        int     dim (size_t axis) const { return (P[axis].size()); }
        float   vox (size_t axis) const { return (D.vox (axis)); }
        const Math::Matrix<float>& transform () const { return (_transform); }
        ssize_t stride (size_t axis) const { return (D.stride (axis)); }

        void reset () 
        { 
          memset (x, 0, sizeof(size_t)*ndim()); 
          for (size_t a = 0; a < ndim(); ++a)
            D[a] = P[a][0]; 
        }

        Position<Extract<Set> > operator[] (size_t axis) { return (Position<Extract<Set> > (*this, axis)); }
        Value<Extract<Set> > value () { return (Value<Extract<Set> > (*this)); }

      private:
        Set& D;
        size_t* x;
        const std::vector<std::vector<int> > P;
        Math::Matrix<float> _transform;

        value_type get_value () const { return (D.value()); }
        void set_value (value_type val) { D.value (val); }
        ssize_t get_pos (size_t axis) const { return (x[axis]); } 
        void set_pos (size_t axis, ssize_t position) const
        {
          x[axis] = position; 
          D[axis] = P[axis][position]; 
        }

        void move_pos (size_t axis, ssize_t increment) const 
        {
          int prev = x[axis]; 
          x[axis] += increment; 
          D[axis] += P[axis][x[axis]] - P[axis][prev]; 
        }

        friend class Position<Extract<Set> >;
        friend class Value<Extract<Set> >;
    };
    
    //! @}
  }
}

#endif


