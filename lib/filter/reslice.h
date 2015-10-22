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

#ifndef __filter_reslice_h__
#define __filter_reslice_h__

#include "adapter/reslice.h"
#include "algo/threaded_copy.h"
#include "datatype.h"

namespace MR
{
  namespace Filter
  {

    //! convenience function to regrid one Image onto another
    /*! This function resamples (regrids) the Image \a source onto the
     * Image& \a destination, using the templated interpolator class.
     *
     * A linear transformation can be optionally applied (that maps from the destination to the source)
     *
     * For example:
     * \code
     * // source and destination data:
     * auto source = Image<float>::create (argument[0]);
     *
     * auto template_header = Image::header (argument[1]);
     * auto destination = Image<float>::create (argument[2], template_header);
     *
     * // regrid source onto destination using linear interpolation:
     * Image::Filter::reslice<Interp::Linear> (source, destination);
     * \endcode
     */
    template <template <class ImageType> class Interpolator, class ImageTypeDestination, class ImageTypeSource>
      void reslice (
          ImageTypeSource& source,
          ImageTypeDestination& destination,
          const transform_type& transform = Adapter::NoTransform,
          const std::vector<int>& oversampling = Adapter::AutoOverSample,
          const typename ImageTypeDestination::value_type value_when_out_of_bounds = Transform::default_out_of_bounds_value<typename ImageTypeDestination::value_type>())
      {
        Adapter::Reslice<Interpolator, ImageTypeSource> interp (source, destination.header(), transform, oversampling, value_when_out_of_bounds);
        threaded_copy_with_progress_message ("reslicing \"" + source.name() + "\"...", interp, destination, 0, source.ndim(), 2);
      }


    //! @}
  }
}

#endif




