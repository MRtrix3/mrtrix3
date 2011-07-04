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

#include "filter/base.h"
#include "dataset/loop.h"
#include "dataset/min_max.h"
#include "data_type.h"
#include "math/golden_section_search.h"

namespace MR
{
  namespace Filter
  {
    namespace
    {

      template <class InputSet> class ImageCorrelationCostFunction {

        public:
          ImageCorrelationCostFunction (InputSet& DataSet) {
            input_image_ = &DataSet;
            init();
          }

          double evaluate_function (double threshold) {
            double sum = 0;
            double mean_xy = 0;
            DataSet::LoopInOrder loop (*input_image_);
            for (loop.start (*input_image_); loop.ok(); loop.next (*input_image_)) {
              if (input_image_->value() > threshold) {
                sum += 1;
                mean_xy += (input_image_->value() * 1);
              } else {
                mean_xy += (input_image_->value() * 0);
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
              for (uint d = 0; d < input_image_->ndim(); d++)
                voxel_count_ *= input_image_->dim(d);

              double sum_sqr = 0, sum = 0;
              DataSet::LoopInOrder loop (*input_image_);
              for (loop.start (*input_image_); loop.ok(); loop.next (*input_image_)) {
                sum_sqr += (input_image_->value() * input_image_->value());
                sum += input_image_->value();
              }
              input_image_mean_ = sum / voxel_count_;
              input_image_stdev_ = sqrt((sum_sqr - sum * input_image_mean_) / voxel_count_);
            }

            InputSet* input_image_;
            uint voxel_count_;
            double input_image_mean_;
            double input_image_stdev_;
        };
    }

    /** \addtogroup Filters
    @{ */

    //! a filter to compute the optimal threshold to mask a DataSet.
    /*! This function computes the optimal threshold to mask a DataSet using the parameter
     *  free approach defined in: Ridgway G et al. (2009) NeuroImage.44(1):99-111.
     *
     * Typical usage:
     * \code
     *
     * Image::Header input_header (argument[0]);
     * Image::Voxel<float> input_voxel (input_header);
     *
     * Image::Header mask_header (input_header);
     * mask_header.set_datatype(DataType::Int8); // Set the output data type
     *
     * Filter::OptimalThreshold<Image::Voxel<float>, Image::Voxel<int> > opt_thres_filter(input_voxel);
     * mask_header.set_params(opt_thres_filter.get_output_params();
     * mask_header.create(argument[1]);
     *
     * Image::Voxel<int> mask_voxel (mask_header);
     * opt_thres_filter.execute(input_voxel, mask_voxel);
     * \endcode
     */
    template <class InputSet, class OutputSet> class OptimalThreshold :
      public Base<InputSet, OutputSet>
    {

    public:
      OptimalThreshold (InputSet& DataSet) :
        Base<InputSet, OutputSet>(DataSet) {
        input_image_ = &DataSet;
      }

      void execute (OutputSet& output) {
        double min, max;
        DataSet::min_max(*input_image_, min, max);

        double optimal_threshold = 0;
        {
          ImageCorrelationCostFunction<InputSet> cost_function(*input_image_);
          Math::GoldenSectionSearch<ImageCorrelationCostFunction<InputSet>,double> golden_search(cost_function, "optimising threshold...");
          optimal_threshold = golden_search.run(min + 0.001*(max-min), (min+max)/2.0 , max-0.001*(max-min));
        }

        DataSet::LoopInOrder threshold_loop (*input_image_, "thresholding...");
        for (threshold_loop.start (*input_image_,output); threshold_loop.ok(); threshold_loop.next (*input_image_,output)) {
          if (finite (input_image_->value()) && input_image_->value() > optimal_threshold)
            output.value() = 1;
        }
      }

    private:
      InputSet* input_image_;
    };
    //! @}
  }
}



#endif
