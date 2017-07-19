/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
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
          const vector<int>& oversampling = Adapter::AutoOverSample,
          const typename ImageTypeDestination::value_type value_when_out_of_bounds = Interp::Base<ImageTypeDestination>::default_out_of_bounds_value())
      {
        Adapter::Reslice<Interpolator, ImageTypeSource> interp (source, destination, transform, oversampling, value_when_out_of_bounds);
        threaded_copy_with_progress_message ("reslicing \"" + source.name() + "\"", interp, destination, 0, source.ndim(), 2);
      }


    //! @}
  }
}

#endif




