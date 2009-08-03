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


    //! %get the voxel data stored at the current position
    /*! This sets the parameters \p val and \p val_im using the voxel data at the current image position, according the \p format speficier.
     * \note If \p format refers to a complex data type, no check is performed to ensure the image is complex. 
     * In this case, calling this function on real-valued data will produce undefined results */
    template <class DataSet> inline void value (const DataSet& ds, OutputType format, float& val, float& val_im)
    {
      switch (format) {
        case Default:   val = ds.value(); return;
        case Real:      val = ds.real(); return;
        case Imaginary: val = ds.imag(); return;
        case Magnitude: val = abs(ds.Z()); return;
        case Phase:     val = arg(ds.Z()); return;
        case RealImag:  val = ds.real(); val_im = ds.imag(); return;
      }
      val = val_im = NAN;
      assert (false);
    }




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
        D[axis]++;
        if (D[axis] < ssize_t(D.dim(axis))) return (true);
        D[axis] = 0;
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
  


    //! @}
  }
}

#endif

