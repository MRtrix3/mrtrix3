/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 8/06/2011

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

#ifndef __filter_optimal_threshold_h__
#define __filter_optimal_threshold_h__

#include "memory.h"
#include "image.h"
#include "algo/threaded_loop.h"
#include "algo/min_max.h"
#include "adapter/replicate.h"
#include "filter/base.h"
#include "math/golden_section_search.h"


namespace MR
{
  namespace Filter
  {

      //! \cond skip
      namespace {

          class MeanStdFunctor {
            public:
              MeanStdFunctor (double& overall_sum, double& overall_sum_sqr, size_t& overall_count) :
                overall_sum (overall_sum), overall_sum_sqr (overall_sum_sqr), overall_count (overall_count),
                sum (0.0), sum_sqr (0.0), count (0) { }

              ~MeanStdFunctor () {
                std::lock_guard<std::mutex> lock (mutex);
                overall_sum += sum;
                overall_sum_sqr += sum_sqr;
                overall_count += count;
              }

              template <class ImageType, class MaskType>
              void operator() (ImageType& vox, MaskType& mask) {
                if (mask.value()) {
                  double in = vox.value();
                  if (std::isfinite(in)) {
                    sum += in;
                    sum_sqr += Math::pow2 (in);
                    ++count;
                  }
                }
              }

              template <class ImageType>
              void operator() (ImageType& vox) {
                  double in = vox.value();
                  if (std::isfinite(in)) {
                    sum += in;
                    sum_sqr += Math::pow2 (in);
                    ++count;
                  }
              }

              double& overall_sum;
              double& overall_sum_sqr;
              size_t& overall_count;
              double sum, sum_sqr;
              size_t count;

              static std::mutex mutex;
          };
          std::mutex MeanStdFunctor::mutex;

          class CorrelationFunctor {
            public:
              CorrelationFunctor (double threshold, double& overall_sum, double& overall_mean_xy) : 
                threshold (threshold), overall_sum (overall_sum), overall_mean_xy (overall_mean_xy),
                sum (0), mean_xy (0.0) { }

              ~CorrelationFunctor () {
                std::lock_guard<std::mutex> lock (mutex);
                overall_sum += sum;
                overall_mean_xy += mean_xy;
              }

              template <class ImageType>
              void operator() (ImageType& vox) {
                double in = vox.value();
                if (std::isfinite(in)) {
                  if (in > threshold) {
                    sum += 1;
                    mean_xy += in;
                  }
                }
              }

              template <class ImageType, class MaskType>
              void operator() (ImageType& vox, MaskType& mask) {
                if (mask.value()) {
                  double in = vox.value();
                  if (std::isfinite(in)) {
                    if (in > threshold) {
                      sum += 1;
                      mean_xy += in;
                    }
                  }
                }
              }

              const double threshold;
              double& overall_sum;
              double& overall_mean_xy;
              double sum;
              double mean_xy;

              static std::mutex mutex;
          };
          std::mutex CorrelationFunctor::mutex;

      }
      //! \endcond


      template <class ImageType, class MaskType>
        class ImageCorrelationCostFunction {

          public:
            typedef typename ImageType::value_type value_type;
            typedef typename MaskType::value_type mask_value_type;

            ImageCorrelationCostFunction (ImageType& input, MaskType* mask = NULL) :
              input (input),
              mask (mask) {
                double sum_sqr = 0.0, sum = 0.0;
                count = 0;

                if (mask) {
                  Adapter::Replicate<MaskType> replicated_mask (*mask, input);
                  ThreadedLoop (input).run (MeanStdFunctor (sum, sum_sqr, count), input, replicated_mask);
                }
                else {
                  ThreadedLoop (input).run (MeanStdFunctor (sum, sum_sqr, count), input);
                }

                input_image_mean = sum / count;
                input_image_stdev = sqrt ((sum_sqr - sum * input_image_mean) / count);
              }

            value_type operator() (value_type threshold) const {
              double sum = 0;
              double mean_xy = 0.0;

              if (mask) {
                Adapter::Replicate<MaskType> replicated_mask (*mask, input);
                ThreadedLoop (input).run (CorrelationFunctor (threshold, sum, mean_xy), input, replicated_mask);
              }
              else
                ThreadedLoop (input).run (CorrelationFunctor (threshold, sum, mean_xy), input);

              mean_xy /= count;
              double covariance = mean_xy - (sum / count) * input_image_mean;
              double mask_stdev = sqrt ((sum - double (sum * sum) / count) / count);

              return -covariance / (input_image_stdev * mask_stdev);
            }

          private:
            ImageType& input;
            MaskType* mask;
            size_t count;
            double input_image_mean;
            double input_image_stdev;
        };


      template <class ImageType, class MaskType>
        typename ImageType::value_type estimate_optimal_threshold (ImageType& input, MaskType* mask)
        {
          typedef typename ImageType::value_type input_value_type;

          input_value_type min, max;
          min_max (input, min, max);

          input_value_type optimal_threshold = 0.0;
          {
            ImageCorrelationCostFunction<ImageType, MaskType> cost_function (input, mask);
            optimal_threshold = Math::golden_section_search (cost_function, "optimising threshold...", 
                min + 0.001*(max-min), (min+max)/2.0 , max-0.001*(max-min));
          }

          return optimal_threshold;
        }




      template <class ImageType>
        inline typename ImageType::value_type estimate_optimal_threshold (ImageType& input)
        {
          return estimate_optimal_threshold (input, (Image<bool>*)nullptr);
        }

      /** \addtogroup Filters
        @{ */

      //! a filter to compute the optimal threshold to mask a DataSet.
      /*! This function computes the optimal threshold to mask a DataSet using the parameter
       *  free approach defined in: Ridgway G et al. (2009) NeuroImage.44(1):99-111.
       *
       * Typical usage:
       * \code
       * Buffer<value_type> input_data (argument[0]);
       * auto input_voxel = input_data.voxel();
       *
       * Filter::OptimalThreshold filter (input_data);
       * Header mask_header (input_data);
       * mask_header.info() = filter.info();
       *
       * Buffer<bool> mask_data (mask_header, argument[1]);
       * auto mask_voxel mask_data.voxel();
       *
       * filter(input_voxel, mask_voxel);
       *
       * \endcode
       */
      class OptimalThreshold : public Base
      {
        public:
          OptimalThreshold (const Header& H) :
              Base (H)
          {
            datatype_ = DataType::Bit;
          }


          template <class InputImageType, class OutputImageType, class MaskType = Image<bool>>
            void operator() (InputImageType& input, OutputImageType& output, MaskType* mask = nullptr)
            {
              axes_.resize (4);
              typedef typename InputImageType::value_type input_value_type;
              input_value_type optimal_threshold = estimate_optimal_threshold (input, mask);
              
              auto f = [&](decltype(input) in, decltype(output) out) {
                input_value_type val = in.value();
                out.value() = ( std::isfinite (val) && val > optimal_threshold ) ? 1 : 0;
              };
              ThreadedLoop ("thresholding...", input) .run (f, input, output);
            }
      };
      //! @}
  }
}




#endif
