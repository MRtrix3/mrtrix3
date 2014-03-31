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

#ifndef __image_filter_optimal_threshold_h__
#define __image_filter_optimal_threshold_h__

#include "ptr.h"
#include "image/buffer_scratch.h"
#include "image/loop.h"
#include "image/min_max.h"
#include "image/adapter/replicate.h"
#include "image/filter/base.h"
#include "math/golden_section_search.h"


namespace MR
{
  namespace Image
  {
    namespace Filter
    {

      //! \cond skip
      namespace {

          class MeanStdFunctor {
            public:
              MeanStdFunctor (double& overall_sum, double& overall_sum_sqr) : 
                overall_sum (overall_sum), overall_sum_sqr (overall_sum_sqr),
                sum (0.0), sum_sqr (0.0) { }

              ~MeanStdFunctor () {
                overall_sum += sum;
                overall_sum_sqr += sum_sqr;
              }

              template <class VoxelType>
                void operator() (VoxelType& vox) {
                  double in = vox.value();
                  sum += in;
                  sum_sqr += Math::pow2 (in);
                }

              double& overall_sum;
              double& overall_sum_sqr;
              double sum, sum_sqr;
          };

          class MeanStdFunctorMask {
            public:
              MeanStdFunctorMask (double& overall_sum, double& overall_sum_sqr, size_t& overall_count) : 
                overall_sum (overall_sum), overall_sum_sqr (overall_sum_sqr), overall_count (overall_count),
                sum (0.0), sum_sqr (0.0), count (0) { }

              ~MeanStdFunctorMask () {
                overall_sum += sum;
                overall_sum_sqr += sum_sqr;
                overall_count += count;
              }

        template <class VoxelType, class MaskVoxelType>
              void operator() (VoxelType vox, MaskVoxelType mask) {
                if (mask.value()) {
                  double in = vox.value();
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
          };

          class CorrelationFunctor {
            public:
              CorrelationFunctor (double threshold, double& overall_sum, double& overall_mean_xy) : 
                threshold (threshold), overall_sum (overall_sum), overall_mean_xy (overall_mean_xy),
                sum (0), mean_xy (0.0) { }

              ~CorrelationFunctor () {
                overall_sum += sum;
                overall_mean_xy += mean_xy;
              }

              template <class VoxelType>
              void operator() (VoxelType& vox) {
                double in = vox.value();
                if (in > threshold) {
                  sum += 1;
                  mean_xy += in;
                }
              }

              const double threshold;
              double& overall_sum;
              double& overall_mean_xy;
              double sum;
              double mean_xy;
          };


          class CorrelationFunctorMask {
            public:
              CorrelationFunctorMask (double threshold, double& overall_sum, double& overall_mean_xy) : 
                threshold (threshold), overall_sum (overall_sum), overall_mean_xy (overall_mean_xy),
                sum (0), mean_xy (0.0) { }

              ~CorrelationFunctorMask () {
                overall_sum += sum;
                overall_mean_xy += mean_xy;
              }

              template <class VoxelType, class MaskVoxelType>
                void operator() (VoxelType& vox, MaskVoxelType& mask) {
                  if (mask.value()) {
                    double in = vox.value();
                    if (in > threshold) {
                      sum += 1;
                      mean_xy += in;
                    }
                  }
                }

              const double threshold;
              double& overall_sum;
              double& overall_mean_xy;
              double sum;
              double mean_xy;
          };


      }
      //! \endcond


      template <class InputVoxelType, class MaskVoxelType>
        class ImageCorrelationCostFunction {

          public:
            typedef typename InputVoxelType::value_type value_type;
            typedef typename MaskVoxelType::value_type mask_value_type;

            ImageCorrelationCostFunction (InputVoxelType& input, MaskVoxelType* mask = NULL) :
              input (input),
              mask (mask) {
                double sum_sqr = 0.0, sum = 0.0;
                count = 0;

                Image::ThreadedLoop loop (input);
                if (mask) {
                  Adapter::Replicate<MaskVoxelType> replicated_mask (*mask, input);
                  loop.run (MeanStdFunctorMask (sum, sum_sqr, count), input, replicated_mask);
                }
                else {
                  loop.run (MeanStdFunctor (sum, sum_sqr), input);
                  count = Image::voxel_count (input);
                }

                input_image_mean = sum / count;
                input_image_stdev = sqrt ((sum_sqr - sum * input_image_mean) / count);
              }

            value_type operator() (value_type threshold) const {
              double sum = 0;
              double mean_xy = 0.0;

              Image::ThreadedLoop loop (input);
              if (mask) {
                  Adapter::Replicate<MaskVoxelType> replicated_mask (*mask, input);
                  loop.run (CorrelationFunctorMask (threshold, sum, mean_xy), input, replicated_mask);
              }
              else
                loop.run (CorrelationFunctor (threshold, sum, mean_xy), input);

              mean_xy /= count;
              double covariance = mean_xy - (sum / count) * input_image_mean;
              double mask_stdev = sqrt ((sum - double (sum * sum) / count) / count);
              return -covariance / (input_image_stdev * mask_stdev);
            }

          private:
            InputVoxelType& input;
            MaskVoxelType* mask;
            size_t count;
            double input_image_mean;
            double input_image_stdev;
        };


      template <class InputVoxelType, class MaskVoxelType> 
        typename InputVoxelType::value_type estimate_optimal_threshold (InputVoxelType& input, MaskVoxelType* mask)
        {
          typedef typename InputVoxelType::value_type input_value_type;

          input_value_type min, max;
          Image::min_max (input, min, max);

          input_value_type optimal_threshold = 0.0;
          {
            ImageCorrelationCostFunction<InputVoxelType, MaskVoxelType> cost_function (input, mask);
            optimal_threshold = Math::golden_section_search (cost_function, "optimising threshold...", 
                min + 0.001*(max-min), (min+max)/2.0 , max-0.001*(max-min));
          }

          return optimal_threshold;
        }




      template <class InputVoxelType> 
        inline typename InputVoxelType::value_type estimate_optimal_threshold (InputVoxelType& input)
        {
          typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
          return estimate_optimal_threshold (input, (BogusMaskType*) NULL);
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
       * Buffer<value_type>::voxel_type input_voxel (input_data);
       *
       * Filter::OptimalThreshold filter (input_data);
       * Header mask_header (input_data);
       * mask_header.info() = filter.info();
       *
       * Buffer<bool> mask_data (mask_header, argument[1]);
       * Buffer<bool>::voxel_type mask_voxel (mask_data);
       *
       * filter(input_voxel, mask_voxel);
       *
       * \endcode
       */
      class OptimalThreshold : public Base
      {
        public:
          template <class InfoType>
          OptimalThreshold (const InfoType& info) :
              Base (info)
          {
            datatype_ = DataType::Bit;
          }


          template <class InputVoxelType, class OutputVoxelType>
            void operator() (InputVoxelType& input, OutputVoxelType& output) {
              typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
              operator() <InputVoxelType, OutputVoxelType, BogusMaskType> (input, output);
            }

          template <class InputVoxelType, class OutputVoxelType, class MaskVoxelType>
            void operator() (InputVoxelType& input, OutputVoxelType& output, MaskVoxelType* mask = NULL) 
            {
              axes_.resize (4);
              typedef typename InputVoxelType::value_type input_value_type;
              input_value_type optimal_threshold = estimate_optimal_threshold (input, mask);
              
              Image::LoopInOrder threshold_loop (input, "thresholding...");
              for (threshold_loop.start (input, output); threshold_loop.ok(); threshold_loop.next (input, output)) {
                input_value_type val = input.value();
                output.value() = ( std::isfinite (val) && val > optimal_threshold ) ? 1 : 0;
              }
            }
      };
      //! @}
    }
  }
}




#endif
