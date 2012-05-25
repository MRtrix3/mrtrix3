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

#include "image/loop.h"
#include "image/min_max.h"
#include "math/golden_section_search.h"
#include "image/buffer_scratch.h"
#include "ptr.h"

namespace MR
{
  namespace Image
  {
    namespace Filter
    {


      template <class InputVoxelType, class MaskVoxelType>
      class ImageCorrelationCostFunction {

        public:

          ImageCorrelationCostFunction (InputVoxelType& DataSet, Ptr<MaskVoxelType>& mask) :
            input_image_ (DataSet) {
            double sum_sqr = 0, sum = 0;
            Image::LoopInOrder loop (input_image_);
            if (mask) {
              mask_ptr = mask;
              voxel_count_ = 0;
              for (loop.start (input_image_); loop.ok(); loop.next (input_image_)) {
                (*mask_ptr)[0] = input_image_[0];
                (*mask_ptr)[1] = input_image_[1];
                (*mask_ptr)[2] = input_image_[2];
                if (mask_ptr->value()) {
                  voxel_count_++;
                  sum_sqr += (input_image_.value() * input_image_.value());
                  sum += input_image_.value();
                }
              }
            } else {
              voxel_count_ = 1;
              for (size_t d = 0; d < input_image_.ndim(); d++)
                voxel_count_ *= input_image_.dim(d);
              Image::LoopInOrder loop (input_image_);
              for (loop.start (input_image_); loop.ok(); loop.next (input_image_)) {
                sum_sqr += (input_image_.value() * input_image_.value());
                sum += input_image_.value();
              }
              }
            input_image_mean_ = sum / voxel_count_;
            input_image_stdev_ = sqrt((sum_sqr - sum * input_image_mean_) / voxel_count_);
          }

          double operator() (double threshold) const{
            double sum = 0;
            double mean_xy = 0;
            Image::LoopInOrder loop (input_image_);
            for (loop.start (input_image_); loop.ok(); loop.next (input_image_)) {
              if (input_image_.value() > threshold) {
                sum += 1;
                mean_xy += (input_image_.value() * 1.0);
              }
            }
            mean_xy /= voxel_count_;
            double covariance = mean_xy - (sum / voxel_count_) * input_image_mean_;
            double mask_stdev = sqrt((sum - sum * sum / voxel_count_) / voxel_count_);
            return -covariance / (input_image_stdev_ * mask_stdev);
          }

        private:
          // Here we pre-compute the input image mean and stdev
          void init() {
            voxel_count_ = 1;
            for (size_t d = 0; d < input_image_.ndim(); d++)
              voxel_count_ *= input_image_.dim(d);

            double sum_sqr = 0, sum = 0;
            Image::LoopInOrder loop (input_image_);
            for (loop.start (input_image_); loop.ok(); loop.next (input_image_)) {
              sum_sqr += (input_image_.value() * input_image_.value());
              sum += input_image_.value();
            }
            input_image_mean_ = sum / voxel_count_;
            input_image_stdev_ = sqrt((sum_sqr - sum * input_image_mean_) / voxel_count_);
          }

          InputVoxelType& input_image_;
          Ptr<MaskVoxelType> mask_ptr;
          size_t voxel_count_;
          double input_image_mean_;
          double input_image_stdev_;
      };

      /** \addtogroup Filters
        @{ */

      //! a filter to compute the optimal threshold to mask a DataSet.
      /*! This function computes the optimal threshold to mask a DataSet using the parameter
       *  free approach defined in: Ridgway G et al. (2009) NeuroImage.44(1):99-111.
       *
       * Typical usage:
       * \code
       * Data<value_type> input_data (argument[0]);
       * Data<value_type>::voxel_type input_voxel (input_data);
       *
       * Filter::OptimalThreshold filter (input_data);
       * Header mask_header (input_data);
       * mask_header.info() = filter.info();
       *
       * Data<int> mask_data (mask_header, argument[1]);
       * Data<int>::voxel_type mask_voxel (mask_data);
       *
       * filter(input_voxel, mask_voxel);
       *
       * \endcode
       */
      class OptimalThreshold : public ConstInfo
      {

        public:
          template <class InputVoxelType>
            OptimalThreshold (const InputVoxelType & DataSet) :
              ConstInfo (DataSet) { }


          template <class InputVoxelType, class OutputVoxelType>
            void operator() (InputVoxelType& input, OutputVoxelType& output) {
              typedef Image::BufferScratch<bool>::voxel_type BogusMaskType;
              Ptr<BogusMaskType > bogusMask;
              operator() <InputVoxelType, OutputVoxelType, BogusMaskType > (input, output, bogusMask);
          }

          template <class InputVoxelType, class OutputVoxelType, class MaskVoxelType>
            void operator() (InputVoxelType& input, OutputVoxelType& output, Ptr<MaskVoxelType>& mask_ptr) {
              axes_.resize (4);

              double min, max;
              Image::min_max(input, min, max);

              double optimal_threshold = 0;
              {
                ImageCorrelationCostFunction<InputVoxelType, MaskVoxelType > cost_function(input, mask_ptr);
                optimal_threshold = Math::golden_section_search(cost_function, "optimising threshold...", min + 0.001*(max-min), (min+max)/2.0 , max-0.001*(max-min));
              }

              Image::LoopInOrder threshold_loop (input, "thresholding...");
              for (threshold_loop.start (input, output); threshold_loop.ok(); threshold_loop.next (input, output)) {
                if (finite (input.value()) && input.value() > optimal_threshold)
                  output.value() = 1;
              }
            }
      };
      //! @}
    }
  }
}




#endif
