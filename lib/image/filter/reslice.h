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

#ifndef __image_interp_reslice_h__
#define __image_interp_reslice_h__

#include "image/adapter/reslice.h"
#include "image/threaded_copy.h"

namespace MR
{
  namespace Image
  {
    namespace Filter
    {

      //! convenience function to regrid one DataSet onto another.
      /*! This function resamples (regrids) the DataSet \a source onto the
       * DataSet& \a destination, using the Interp::Reslice class.
       *
       * For example:
       * \code
       * // source and destination data:
       * Image::Header source_header (...);
       * Image::Voxel<float> source (source_header);
       *
       * Image::Header destination_header (...);
       * Image::Voxel<float> destination (destination_header);
       *
       * // regrid source onto destination using linear interpolation:
       * DataSet::Interp::reslice<DataSet::Interp::Linear> (destination, source);
       * \endcode
       */
      template <template <class VoxelType> class Interpolator, class VoxelTypeDestination, class VoxelTypeSource>
        void reslice (
            VoxelTypeSource& source,
            VoxelTypeDestination& destination,
            const Math::Matrix<float>& operation = Adapter::NoOp,
            const std::vector<int>& oversampling = Adapter::AutoOverSample)
        {
          Adapter::Reslice<Interpolator,VoxelTypeSource> interp (source, destination, operation, oversampling);
          Image::threaded_copy_with_progress_message ("reslicing \"" + source.name() + "\"...", interp, destination, 2);
        }


      //! @}
    }
  }
}

#endif




