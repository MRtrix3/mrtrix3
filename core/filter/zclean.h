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


#ifndef __filter_zclean_h__
#define __filter_zclean_h__

#include "progressbar.h"
#include "memory.h"
#include "image.h"
#include "algo/copy.h"
#include "algo/loop.h"
#include "filter/base.h"
#include "filter/erode.h"
#include "filter/dilate.h"
#include "filter/connected_components.h"
#include "math/median.h"


namespace MR
{
  namespace Filter
  {



    class ZClean : public Base { MEMALIGN(ZClean)

      public:
        template <class HeaderType>
        ZClean (const HeaderType& in) :
            Base (in),
            zupper (2.5),
            zlower (2.5),
            fov_max (0.3),
            fov_min (0.15),
            bridge (0),
            dont_maskupper (false),
            keep_lower (false),
            keep_upper (true)
        {
          datatype_ = DataType::Float32;
          ndim() = 3;
        }

        template <class HeaderType>
        ZClean (const HeaderType& in, const std::string& message) :
            Base (in, message),
            zupper (2.5),
            zlower (2.5),
            fov_max (0.3),
            fov_min (0.15),
            bridge (0),
            dont_maskupper (false),
            keep_lower (false),
            keep_upper (true)
        {
          datatype_ = DataType::Float32;
          ndim() = 3;
        }

        template <class InputImageType, class MaskType, class OutputImageType>
        void operator() (InputImageType& input, MaskType& spatial_prior, OutputImageType& output)
        {
          if (output.ndim() > 3)
            throw Exception ("3D output expected");

          std::unique_ptr<ProgressBar> progress (message.size() ? new ProgressBar (message) : nullptr);

          Image<bool> int_roi = Image<bool>::scratch (Header(spatial_prior), "temporary initial mask");
          INFO ("creating intensity mask from input mask");
          Dilate dilation_filter (spatial_prior);
          dilation_filter.set_npass(2);
          dilation_filter (spatial_prior, int_roi);
          ssize_t cnt = 0;
          for (auto l = Loop (0,3) (int_roi); l; ++l) {
            cnt += int_roi.value();
          }
          ssize_t cnt_lower = std::max<size_t>(10000, std::floor(fov_min * input.size(0) * input.size(1) * input.size(2)));
          ssize_t cnt_upper = std::floor(fov_max * input.size(0) * input.size(1) * input.size(2));
          float mad, median, previous_mad, previous_median;
          calculate_median_mad<Image<float>, Image<bool>> (input, int_roi, cnt, median, mad);
          INFO ("median: "+str(median));
          INFO ("mad: "+str(mad));
          INFO ("lower: " + str(median - zlower * mad) + " upper: " + str(median + zupper * mad));

          INFO ("eroding intensity mask");
          while (cnt >= cnt_lower) {
            if (progress)
              ++(*progress);
            Erode erosion_filter (int_roi);
            erosion_filter.set_npass(1);
            erosion_filter (int_roi, int_roi);
            cnt = 0;
            for (auto l = Loop (0,3) (int_roi); l; ++l)
              cnt += int_roi.value();
            if (cnt == 0)
              throw Exception ("mask empty after erosion");
            previous_median = median;
            previous_mad = mad;
            calculate_median_mad<Image<float>, Image<bool>> (input, int_roi, cnt, median, mad);
            upper = median + zupper * mad;
            lower = median - zlower * mad;
            INFO ("median: " + str(median) + ", changed: "+str((median - previous_median) / previous_median));
            INFO ("mad: " + str(mad) + ", changed: "+str((mad - previous_mad) / previous_mad));
            INFO ("FOV: " + str(float(cnt) / (input.size(0) * input.size(1) * input.size(2))));
            INFO ("lower: " + str(lower) + " upper: " + str(upper));
            INFO ("cnt_upper - cnt: " + str(cnt_upper - cnt));
            if (lower > 0.0 && ((median + 2.5 * mad) - (previous_median + 2.5 * previous_mad)) < 0.0 && (cnt < cnt_upper))
              break;
          }

          if (App::log_level >= 3) {
            auto masked_image = Image<float>::scratch (input, "robust z score");
            // output;
            for (auto l = Loop (0,3) (masked_image, input, int_roi); l; ++l) {
              if (int_roi.value())
                masked_image.value() = input.value();
            }
            display (masked_image);
          }

          {
            INFO ("intensity sample mask");
            if (progress)
              ++(*progress);

            Image<float> eroded_zscore_image;
            if (App::log_level >= 3) {
              eroded_zscore_image = Image<float>::scratch (input, "robust z score");
            }

            int maxiter = 5;
            while (maxiter--) {
              // refine image mask based on robust Z score
              cnt = 0;
              for (auto l = Loop (0,3) (input, int_roi); l; ++l) {
                if (int_roi.value()) {
                  float z = (input.value() - median) / mad;
                  bool good = (z > -zlower) && (z < zupper);
                  if (App::log_level >= 3) {
                    assign_pos_of(input, 0, 3).to(eroded_zscore_image);
                    eroded_zscore_image.value() = (z > -zlower) && (dont_maskupper || z < zupper) ? z : NaN;
                  }
                  if (good) cnt++;
                  int_roi.value() = good;
                } else if (App::log_level >= 3) {
                  assign_pos_of(input, 0, 3).to(eroded_zscore_image);
                  eroded_zscore_image.value() = NaN;
                }
              }
              previous_mad = mad;
              previous_median = median;
              calculate_median_mad<Image<float>, Image<bool>> (input, int_roi, cnt, median, mad);
              upper = median + zupper * mad;
              lower = median - zlower * mad;
              INFO("median: " + str(median) + ", changed: " + str((median - previous_median)));
              INFO("mad: " + str(mad) + ", changed: " + str((mad - previous_mad)));
              INFO("lower: " + str(lower) + " upper: " + str(upper));
              float change = std::abs(median - previous_median) / previous_mad;
              INFO("convergence: "+str(change));
              if (change < 1e-2)
                break;
            }
            if (App::log_level >= 3)
              display (eroded_zscore_image);
          }
          upper = median + zupper * mad;
          lower = median - zlower * mad;
          if (lower < 0.0) {
            WARN ("likely not converged, setting lower to 0.0");
            lower = 0.0;
          }


          INFO ("lower: "+str(lower));
          INFO ("upper: "+str(upper));
          INFO ("bridge: "+str(bridge));

          mask = Image<bool>::scratch (Header(spatial_prior), "temporary mask");
          if (progress)
            ++(*progress);

          for (auto l = Loop (0,3) (input, mask, spatial_prior); l; ++l)
            mask.value() = spatial_prior.value() && input.value() >= lower && (dont_maskupper || input.value() <= upper);

          if (App::log_level >= 3)
            display (mask);
          if (progress)
            ++(*progress);

          {
            INFO ("selecting largest ROI");
            ConnectedComponents connected_filter (mask);
            connected_filter.set_largest_only (true);
            connected_filter (mask, mask);
            if (progress)
              ++(*progress);
          }

          for (auto l = Loop (0,3) (mask); l; ++l)
            mask.value() = !mask.value();

          {
            INFO ("removing masked out islands");
            ConnectedComponents connected_filter (mask);
            connected_filter.set_largest_only (true);
            connected_filter (mask, mask);
            if (progress)
              ++(*progress);
          }

          if (bridge) {
            INFO ("bridging");
            for (auto l = Loop (0,3) (mask); l; ++l)
              mask.value() = !mask.value();
            if (progress)
              ++(*progress);
            Dilate dilation_filter (mask);
            dilation_filter.set_npass(bridge);
            dilation_filter (mask, mask);
            if (progress)
              ++(*progress);
            for (auto l = Loop (0,3) (mask); l; ++l)
              mask.value() = !mask.value();
            if (progress)
              ++(*progress);
            ConnectedComponents connected_filter (mask);
            connected_filter.set_largest_only (true);
            connected_filter (mask, mask);
            if (progress)
              ++(*progress);
            Dilate dilation_filter2 (mask);
            dilation_filter2.set_npass(bridge);
            dilation_filter2 (mask, mask);
            if (progress)
              ++(*progress);
            if (App::log_level >= 3)
              display (mask);
          }

          for (auto l = Loop (0,3) (mask, spatial_prior); l; ++l)
            mask.value() = !mask.value() && spatial_prior.value();
          if (progress)
            ++(*progress);

          float lo = std::max<float>(median - 2.5 * mad, lower);
          float hi = std::min<float>(median + 2.5 * mad, upper);
          for (auto l = Loop (0,3) (input, spatial_prior, mask, output); l; ++l) {
            if (!spatial_prior.value())
              continue;
            float val = input.value();
            if (mask.value()) {
              if (val < lo)
                output.value() = val; // hack
              else if (val > hi)
                output.value() = hi;
              else
                output.value() = val;
              continue;
            } else { // outside refined mask but inside initial mask
              if (keep_lower && val < lo)
                output.value() = lo;
              else if (keep_upper && val > hi)
                output.value() = hi;
            }
          }

        }

        void set_zlim (float upper, float lower)
        {
          zupper = upper;
          zlower = lower;
        }

        void set_voxels_to_bridge (size_t nvoxels)
        {
          bridge = nvoxels;
        }

        Image<bool> mask;

      protected:
        float zupper, zlower;
        float fov_max, fov_min;
        size_t bridge;
        bool dont_maskupper, keep_lower, keep_upper;
        float upper, lower;

        template <typename ImageType, typename MaskType>
        void calculate_median_mad (ImageType& image, MaskType& mask, size_t nvoxels, float& median, float& mad) {
          MR::vector<float> vals (nvoxels);
          size_t idx = 0;
          for (auto l = Loop (0,3) (mask, image); l; ++l) {
            if (mask.value())
              vals[idx++] = image.value();
            assert (idx <= nvoxels);
          }
          median = Math::median(vals);
          for (auto & v : vals)
            v = std::abs(v - median);
          mad = Math::median(vals);
        }

    };
    //! @}
  }
}




#endif
