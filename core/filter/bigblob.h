/* Copyright (c) 2008-2020 the MRtrix3 contributors.
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

#ifndef __filter_bigblob_h__
#define __filter_bigblob_h__

#include "image.h"
#include "algo/loop.h"
#include "filter/base.h"
#include "filter/connected_components.h"


namespace MR
{
  namespace Filter
  {

    /** \addtogroup Filters
      @{ */

    //! a filter to obtain the filled largest connected component ("blob")
    /* Selects the largest connected component in a mask image, and then
     * fills any holes within that component
     *
     * Typical usage:
     * \code
     * auto input = Image<bool>::open (argument[0]);
     * Filter::BigBlob bigblob_filter (input);
     * auto output = Image<bool>::create (argument[1], bigblob_filter);
     * bigblob_filter (input, output);
     *
     * \endcode
     */
    class BigBlob : public Base { MEMALIGN(BigBlob)

      public:
        template <class HeaderType>
        BigBlob (const HeaderType& in) :
            Base (in)
        {
          check_3D_nonunity (in);
          datatype_ = DataType::Bit;
        }

        template <class HeaderType>
        BigBlob (const HeaderType& in, const std::string& message) :
            Base (in, message)
        {
          check_3D_nonunity (in);
          datatype_ = DataType::Bit;
        }

        template <class InputImageType, class OutputImageType>
        void operator() (InputImageType& input, OutputImageType& output)
        {
          std::unique_ptr<ProgressBar> progress (message.size() ? new ProgressBar (message, 6) : nullptr);
          if (progress)
            ++(*progress);
          auto temp_image = Image<bool>::scratch (input, "temporary mask");
          if (progress)
            ++(*progress);
          ConnectedComponents connected_filter (temp_image);
          connected_filter.set_largest_only (true);
          connected_filter (input, temp_image);
          if (progress)
            ++(*progress);
          for (auto l = Loop (temp_image) (temp_image); l; ++l)
            temp_image.value() = !temp_image.value();
          if (progress)
            ++(*progress);
          connected_filter (temp_image, temp_image);
          if (progress)
            ++(*progress);
          for (auto l = Loop (temp_image) (temp_image, output); l; ++l)
            output.value() = !temp_image.value();
        }


    };
    //! @}
  }
}




#endif
