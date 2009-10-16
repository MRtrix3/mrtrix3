/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 16/10/09.

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

#ifndef __dataset_loop_h__
#define __dataset_loop_h__

namespace MR {
  namespace DataSet {

    //! \addtogroup DataSet
    // @{

    namespace Loop {

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



      template <class Functor, class DataSet> inline void all (Functor& func, DataSet& ds) {
        assert (voxel_count (ds));
        do func (ds); while (next (ds)); 
      }

      template <class Functor, class DataSet1, class DataSet2> inline void all (Functor& func, DataSet1& ds1, DataSet2& ds2) {
        assert (voxel_count (ds1));
        do func (ds1, ds2); while (next (ds1)); 
      }

    }
    //! @}
  }
}

#endif


