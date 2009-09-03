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

#ifndef __image_misc_h__
#define __image_misc_h__

#include "data_type.h"
#include "math/complex.h"

namespace MR {
  namespace Image {

    //! \addtogroup Image
    // @{


    //! used to loop over all image coordinates.
    /*! This function increments the current position to the next voxel, incrementing to the next axis as required.
     * It is used to process all voxels in turn. For example:
     * \code
     * Image::Voxel position (image_object);
     * do {
     *   process (position.value());
     * } while (next (position));
     * \endcode
     * \return true once the last voxel has been reached (i.e. the next increment would bring the current position out of bounds), false otherwise. */
    template <class DataSet> inline bool next (DataSet& D)
    {
      size_t axis = 0;
      do {
        D.inc(axis);
        if (D.pos(axis) < ssize_t(D.dim(axis))) return (true);
        D.pos(axis, 0);
        axis++;
      } while (axis < D.ndim());
      return (false);
    }


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


    //! returns the memory footprint of a data set consisting of \a num_voxel voxels stored as objects of type \a dt
    inline off64_t memory_footprint (const DataType& dt, off64_t num_voxel)
    {
      if (dt.bits() < 8) return ((num_voxel+7)/8);
      return (dt.bytes() * num_voxel);
    }
  
    //! returns the memory footprint of a DataSet
    template <class DataSet> inline off64_t memory_footprint (const DataSet& ds, size_t up_to_dim = SIZE_MAX) 
    {
      return (memory_footprint (ds.datatype(), voxel_count (ds, up_to_dim))); 
    }
  
    //! returns the memory footprint of a DataSet
    template <class DataSet> inline off64_t memory_footprint (const DataSet& ds, const char* specifier)
    {
      return (memory_footprint (ds.datatype(), voxel_count (ds, specifier))); 
    }


    //! @}
  }
}

#endif

