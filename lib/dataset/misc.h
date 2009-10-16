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

/** \defgroup DataSet DataSet functions
 * \brief template functions to operate of DataSet objects
 *
 * These functions provide templates for algorithms that operate on Objects
 * that implement the DataSet interface.
 */

namespace MR {
  namespace DataSet {

    //! \addtogroup DataSet
    // @{

    //! returns the number of voxel in the data set, or a relevant subvolume
    template <class DataSet> inline off64_t voxel_count (const DataSet& ds, size_t up_to_dim = SIZE_MAX) 
    { 
      if (up_to_dim > ds.ndim()) up_to_dim = ds.ndim();
      off64_t fp = 1;
      for (size_t n = 0; n < up_to_dim; n++) 
        fp *= ds.dim(n);
      return (fp); 
    }

    //! returns the number of voxel in the relevant subvolume of the data set 
    template <class DataSet> inline off64_t voxel_count (const DataSet& ds, const char* specifier) 
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
    }
    //! \endcond

    //! return whether the DataSet contains complex data
    template <class DataSet> inline bool is_complex (const DataSet& ds) { 
      typedef typename DataSet::value_type T;
      return (is_complex__<T> ()); 
    }


    //! @}
  }
}

#endif

