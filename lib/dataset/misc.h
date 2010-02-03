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

#ifndef __dataset_misc_h__
#define __dataset_misc_h__

#include "data_type.h"
#include "math/complex.h"

namespace MR {
  namespace DataSet {

    //! \addtogroup DataSet
    // @{

    //! returns the number of voxel in the data set, or a relevant subvolume
    template <class Set> inline off64_t voxel_count (const Set& ds, size_t from_axis = 0, size_t to_axis = SIZE_MAX) 
    { 
      if (to_axis > ds.ndim()) to_axis = ds.ndim();
      assert (from_axis < to_axis);
      off64_t fp = 1;
      for (size_t n = from_axis; n < to_axis; n++) 
        fp *= ds.dim(n);
      return (fp); 
    }

    //! returns the number of voxel in the relevant subvolume of the data set 
    template <class Set> inline off64_t voxel_count (const Set& ds, const char* specifier) 
    { 
      off64_t fp = 1;
      for (size_t n = 0; n < ds.ndim(); n++) 
        if (specifier[n] != ' ') fp *= ds.dim(n);
      return (fp); 
    }


    //! \cond skip
    namespace {
      template <typename T> inline bool is_complex__ () { return (false); }
      template <> inline bool is_complex__<cfloat> () { return (true); }
      template <> inline bool is_complex__<cdouble> () { return (true); }
      template <class Set> class CompareStride {
        public:
          CompareStride (const Set& set) : S (set) { }
          bool operator() (const size_t a, const size_t b) const { return (abs(S.stride(a)) < abs(S.stride(b))); }
        private:
          const Set& S;
      };
    }
    //! \endcond



    //! sort strides in increasing absolute order and return vector of indices
    template <class Set> std::vector<size_t> stride_order (const Set& set) {
      std::vector<size_t> ret (set.ndim());
      for (size_t i = 0; i < ret.size(); ++i) ret[i] = i;
      CompareStride<Set> compare (set);
      std::sort (ret.begin(), ret.end(), compare);
      return (ret);
    }



    //! return whether the Set contains complex data
    template <class Set> inline bool is_complex (const Set& ds) { 
      typedef typename Set::value_type T;
      return (is_complex__<T> ()); 
    }

    template <class Set1, class Set2> inline bool dimensions_match (Set1& D1, Set2& D2) {
      if (D1.ndim() != D2.ndim()) return (false);
      for (size_t n = 0; n < D1.ndim(); ++n)
        if (D1.dim(n) != D2.dim(n)) return (false);
      return (true);
    }

    template <class Set1, class Set2> inline bool dimensions_match (Set1& D1, Set2& D2, size_t from_axis, size_t to_axis) {
      assert (from_axis < to_axis);
      if (to_axis > D1.ndim() || to_axis > D2.ndim()) return (false);
      for (size_t n = from_axis; n < to_axis; ++n)
        if (D1.dim(n) != D2.dim(n)) return (false);
      return (true);
    }


    //! @}
  }
}

#endif

