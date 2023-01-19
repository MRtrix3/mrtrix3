/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
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
          const vector<uint32_t>& oversampling = Adapter::AutoOverSample,
          const typename ImageTypeDestination::value_type value_when_out_of_bounds = Interp::Base<ImageTypeDestination>::default_out_of_bounds_value())
      {
        Adapter::Reslice<Interpolator, ImageTypeSource> interp (source, destination, transform, oversampling, value_when_out_of_bounds);
        threaded_copy_with_progress_message ("reslicing \"" + source.name() + "\"", interp, destination, 0, source.ndim(), 2);
      }


    //! @}
  }
}

#endif




