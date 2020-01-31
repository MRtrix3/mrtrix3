/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#ifndef __filter_mask_clean_h__
#define __filter_mask_clean_h__

#include "progressbar.h"
#include "memory.h"
#include "image.h"
#include "image_helpers.h"
#include "algo/copy.h"
#include "algo/loop.h"
#include "filter/base.h"
#include "filter/erode.h"
#include "filter/connected_components.h"
#include "filter/dilate.h"



namespace MR
{
  namespace Filter
  {

    /** \addtogroup Filters
      @{ */

    //! a filter to clean up masks typically output by DWIBrainMask filter
    /*! Removes peninsula-like extensions of binary masks, where the
     *  peninsula itself is wider than the bridge connecting it to the mask.
     *  Typical examples are eyes connected to the mask by parts of the
     *  optical nerves; or other non-brain parts or artefacts.
     *
     * Typical usage:
     * \code
     * auto input = Image<bool>::open (argument[0]);
     * Filter::MaskClean mask_clean_filter (input);
     * auto output = Image<bool>::create (argument[1], mask_clean_filter);
     * mask_clean_filter (input, output);
     *
     * \endcode
     */
    class MaskClean : public Base { MEMALIGN(MaskClean)

      public:
        template <class HeaderType>
        MaskClean (const HeaderType& in) :
            Base (in),
            scale (2)
        {
          check_3D_nonunity (in);
          datatype_ = DataType::Bit;
        }

        template <class HeaderType>
        MaskClean (const HeaderType& in, const std::string& message) :
            Base (in, message),
            scale (2)
        {
          check_3D_nonunity (in);
          datatype_ = DataType::Bit;
        }

        template <class InputImageType, class OutputImageType>
        void operator() (InputImageType& input, OutputImageType& output)
        {

          std::unique_ptr<ProgressBar> progress (message.size() ? new ProgressBar (message) : nullptr);
          
          if (progress)
            ++(*progress);

          auto temp_in = Image<bool>::scratch (input, "temporary input mask");
          for (auto l = Loop (0,3) (input, temp_in); l; ++l)
            temp_in.value() = input.value();

          auto temp_out = Image<bool>::scratch (input, "temporary output mask");

          single_pass(temp_in, temp_out);

          if (progress)
            ++(*progress);

          // Perform extra passes until converged.
          while (differ(temp_in, temp_out)) {

            for (auto l = Loop (0,3) (temp_out, temp_in); l; ++l)
              temp_in.value() = temp_out.value();

            single_pass(temp_in, temp_out);

            if (progress)
              ++(*progress);

          }

          for (auto l = Loop (0,3) (temp_out, output); l; ++l)
            output.value() = temp_out.value();

        }


        void set_scale (unsigned int scales)
        {
          scale = scales;
        }


      protected:

        // Single pass over all scales + retaining the largest connected component to clean up remains.
        template <class InputImageType, class OutputImageType>
        void single_pass (InputImageType& input, OutputImageType& output)
        {
          auto temp_image = Image<bool>::scratch (input, "temporary mask");
          for (auto l = Loop (0,3) (input, temp_image); l; ++l)
            temp_image.value() = input.value();

          for (unsigned int s = scale; s > 0; --s)
            single_scale(temp_image, temp_image, s);

          ConnectedComponents connected_filter (temp_image);
          connected_filter.set_largest_only (true);
          connected_filter (temp_image, temp_image);

          for (auto l = Loop (0,3) (temp_image, output); l; ++l)
            output.value() = temp_image.value();
        }
        
        // Core operation for a single specific scale s.
        template <class InputImageType, class OutputImageType>
        void single_scale (InputImageType& input, OutputImageType& output, const int ss)
        {
          auto del_image = Image<bool>::scratch (input, "deletion mask");
          Erode erosion_filter (input);
          erosion_filter.set_npass(ss);
          erosion_filter (input, del_image);

          auto largest_image = Image<bool>::scratch (input, "largest component");
          ConnectedComponents connected_filter (del_image);
          connected_filter.set_largest_only (true);
          connected_filter (del_image, largest_image);

          for (auto l = Loop (0,3) (del_image, largest_image); l; ++l)
            if (largest_image.value())
              del_image.value() = 0;

          Dilate dilation_filter (del_image);
          dilation_filter.set_npass(ss+1);
          dilation_filter (del_image, del_image);

          for (auto l = Loop (0,3) (input, largest_image, del_image); l; ++l)
            largest_image.value() = del_image.value() ? 0 : input.value();

          for (auto l = Loop (0,3) (largest_image, output); l; ++l)
            output.value() = largest_image.value();
        }

        template <class InputImageType, class OutputImageType>
        bool differ (InputImageType& ima, OutputImageType& imb)
        {
          for (auto l = Loop (0,3) (ima, imb); l; ++l)
            if (ima.value() != imb.value())
              return true;
            
          return false;
        }

        unsigned int scale;
    };
    //! @}
  }
}




#endif
