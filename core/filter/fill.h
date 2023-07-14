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

#ifndef __filter_fill_h__
#define __filter_fill_h__

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

    //! a filter to fill the interior of a mask image
    /* Fill operation is performed by inverting the mask, selecting
     * the largest connected component, and inverting the result
     *
     * Typical usage:
     * \code
     * auto input = Image<bool>::open (argument[0]);
     * Filter::Fill fill_filter (input);
     * auto output = Image<bool>::create (argument[1], fill_filter);
     * fill_filter (input, output);
     *
     * \endcode
     */
    class Fill : public Base { 

      public:
        template <class HeaderType>
        Fill (const HeaderType& in) :
            Base (in),
            enabled_axes (ndim(), true),
            do_26_connectivity (false)
        {
          check_3D_nonunity (in);
          datatype_ = DataType::Bit;
        }

        template <class HeaderType>
        Fill (const HeaderType& in, const std::string& message) :
            Fill (in)
        {
          set_message (message);
        }

        void set_axes (const vector<int>& i)
        {
          const size_t max_axis = *std::max_element (i.begin(), i.end());
          if (max_axis >= ndim())
            throw Exception ("Requested axis for interior-filling filter (" + str(max_axis) + " is beyond the dimensionality of the image (" + str(ndim()) + "D)");
          enabled_axes.assign (std::max (max_axis+1, size_t(ndim())), false);
          for (const auto& axis : i) {
            if (axis < 0)
              throw Exception ("Cannot specify negative axis index for interior-filling filter");
            enabled_axes[axis] = true;
          }
        }

        void set_26_connectivity (bool value)
        {
          do_26_connectivity = value;
        }

        template <class InputImageType, class OutputImageType>
        void operator() (InputImageType& input, OutputImageType& output)
        {
          std::unique_ptr<ProgressBar> progress (message.size() ? new ProgressBar (message, 3) : nullptr);
          auto temp_image = Image<bool>::scratch (input, "scratch mask for interior-filling filter");
          for (auto l = Loop (input) (input, temp_image); l; ++l)
            temp_image.value() = !input.value();
          if (progress)
            ++(*progress);
          ConnectedComponents connected_filter (temp_image);
          connected_filter.set_axes (enabled_axes);
          connected_filter.set_largest_only (true);
          connected_filter.set_26_connectivity (do_26_connectivity);
          connected_filter (temp_image, temp_image);
          if (progress)
            ++(*progress);
          for (auto l = Loop (temp_image) (temp_image, output); l; ++l)
            output.value() = !temp_image.value();
        }


      protected:
        vector<bool> enabled_axes;
        bool do_26_connectivity;

    };
    //! @}
  }
}




#endif
