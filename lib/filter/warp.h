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

#ifndef __filter_warp_h__
#define __filter_warp_h__

#include "adapter/warp.h"
#include "algo/threaded_copy.h"
#include "datatype.h"
#include "interp/cubic.h"
#include "filter/reslice.h"

namespace MR
{
  namespace Filter
  {

    //! convenience function to warp one DataSet onto another
    /*! This function resamples (regrids) the Image \a source onto the
     * Image& \a destination, using the templated interpolator class and a supplied deformation field.
     *
     * For example:
     * \code
     * // source and destination data:
     * Image::Buffer<float> source_buffer (...);
     * auto source = source_buffer.voxel();
     *
     * Image::Buffer<float> destination_buffer (...);
     * auto destination = destination_buffer.voxel();
     *
     * // regrid source onto destination using linear interpolation:
     * Image::Filter::warp<Image::Interp::Linear> (source, destination, deformation);
     * \endcode
     */
    template <template <class VoxelType> class Interpolator, class ImageTypeDestination, class ImageTypeSource, class WarpType>
      void warp (
          ImageTypeSource& source,
          ImageTypeDestination& destination,
          WarpType& warp,
          const bool reorient_fods = false,
          const transform_type& transform = Adapter::NoTransform,
          const typename ImageTypeDestination::value_type value_when_out_of_bounds = Transform::default_out_of_bounds_value<typename ImageTypeDestination::value_type>())
      {

        // reslice warp onto destination grid
        auto header = destination.header();
        header.set_ndim(4);
        header.size(3) = 3;
        header.stride(0) = 2;
        header.stride(1) = 3;
        header.stride(2) = 4;
        header.stride(3) = 1;
        auto warp_resliced = Image<typename WarpType::value_type>::scratch (header);
        reslice<Interp::Linear> (warp, warp_resliced);

        // compose warp with affine
        Registration::Transform::compose (transform, warp_resliced, warp_resliced);

        // apply warp
        Adapter::Warp<Interpolator, ImageTypeSource, Image<typename WarpType::value_type> > interp (source, warp_resliced, value_when_out_of_bounds);
        threaded_copy_with_progress_message ("warping \"" + source.name() + "\"...", interp, destination);

        // reorient FODs
        if (reorient_fods) {

        }

      }


    //! @}
  }
}

#endif




