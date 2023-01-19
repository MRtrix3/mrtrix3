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

#ifndef __image_filter_resize_h__
#define __image_filter_resize_h__

#include "filter/base.h"
#include "filter/reslice.h"
#include "filter/smooth.h"
#include "interp/nearest.h"
#include "interp/linear.h"
#include "interp/cubic.h"
#include "interp/sinc.h"
#include "image.h"
#include "algo/copy.h"

namespace MR
{
  namespace Filter
  {
    /** \addtogroup Filters
    @{ */

    /*! Resize an image
     *
     *  Note that if the image is 4D, then only the first 3 dimensions can be resized.
     *
     *  Also note that if the image is down-sampled, the appropriate smoothing is automatically applied.
     *  using Gaussian smoothing.
     *
     *  Automatic oversampling is applied except for nearest-neighbour interpolation, which uses no oversampling.
     *
     * Typical usage:
     * \code
     * auto input = Image<default_type>::open (argument[0]);
     * Image::Filter::Resize resize_filter (input);
     * default_type scale = 0.5;
     * resize_filter.set_scale_factor (scale);
     * auto output = Image::create (argument[1], resize_filter);
     *
     * resize_filter (src, dest);
     *
     * \endcode
     */
    class Resize : public Base { MEMALIGN(Resize)

      public:
        template <class HeaderType>
        Resize (const HeaderType& in) :
            Base (in),
            interp_type (2), // Cubic
            transformation (Adapter::NoTransform),
            oversampling (Adapter::AutoOverSample),
            out_of_bounds_value (nullptr) { }

        ~Resize () { delete out_of_bounds_value; }

        void set_voxel_size (default_type size) {
          vector<default_type> voxel_size (3, size);
          set_voxel_size (voxel_size);
        }


        void set_voxel_size (const vector<default_type>& voxel_size) {
          if (voxel_size.size() != 3)
            throw Exception ("the voxel size must be defined using a value for all three dimensions.");

          Eigen::Vector3d original_extent;
          for (size_t j = 0; j < 3; ++j) {
            if (voxel_size[j] <= 0.0)
              throw Exception ("the voxel size must be larger than zero");
            original_extent[j] = axes_[j].size * axes_[j].spacing;
            axes_[j].size = std::round (axes_[j].size * axes_[j].spacing / voxel_size[j] - 0.0001); // round down at .5
            // Here we adjust the translation to ensure the image extent is centered wrt the original extent.
            // This is important when the new voxel size is not an exact multiple of the original extent
            for (size_t i = 0; i < 3; ++i)
              transform_(i,3) += 0.5 * ((voxel_size[j] - axes_[j].spacing)  + (original_extent[j] - (axes_[j].size * voxel_size[j]))) * transform_(i,j);
            axes_[j].spacing = voxel_size[j];
          }
        }


        void set_size (const vector<uint32_t>& image_res) {
          if (image_res.size() != 3)
            throw Exception ("the image resolution must be defined for 3 spatial dimensions");
          vector<default_type> new_voxel_size (3);
          for (size_t d = 0; d < 3; ++d) {
            if (image_res[d] <= 0)
              throw Exception ("the image resolution must be larger than zero for all 3 spatial dimensions");
            new_voxel_size[d] = (this->size(d) * this->spacing(d)) / image_res[d];
          }
          set_voxel_size (new_voxel_size);
        }


        void set_scale_factor (default_type scale) {
          set_scale_factor (vector<default_type> (3, scale));
        }


        void set_scale_factor (const vector<default_type> & scale) {
          if (scale.size() != 3)
            throw Exception ("a scale factor for each spatial dimension is required");
          vector<default_type> new_voxel_size (3);
          for (size_t d = 0; d < 3; ++d) {
            if (scale[d] <= 0.0)
              throw Exception ("the scale factor must be larger than zero");
            new_voxel_size[d] = (this->size(d) * this->spacing(d)) / std::ceil (this->size(d) * scale[d]);
          }
          set_voxel_size (new_voxel_size);
        }

        void set_oversample (vector<uint32_t> oversample) {
          if (oversample.size() == 1)
            oversample.resize (3, oversample[0]);
          else if (oversample.size() != 3 and oversample.size() != 0)
            throw Exception ("FIXME oversample requires either a vector of a 0 (auto), 1 or 3 integers integer, got " + str(oversample.size()));
          for (auto f : oversample) {
            if (f < 1)
              throw Exception ("oversample factors must be positive integers");
          }
          oversampling = oversample;
        }

        void set_interp_type (int type) {
          interp_type = type;
        }

        void set_transform (const transform_type& trafo) {
          transform_ = trafo;
        }

        void set_out_of_bounds_value (default_type value) {
          out_of_bounds_value = new default_type;
          *out_of_bounds_value = value;
        }

        template <class InputImageType, class OutputImageType>
          void operator() (InputImageType& input, OutputImageType& output) {
            const typename InputImageType::value_type oob = out_of_bounds_value ?
             *out_of_bounds_value : Interp::Base<InputImageType>::default_out_of_bounds_value();
            switch (interp_type) {
            case 0:
              // Use of oversampling is prevented in reslice adapter
              reslice <Interp::Nearest> (input, output, transformation, oversampling, oob);
              break;
            case 1:
              reslice <Interp::Linear> (input, output, transformation, oversampling, oob);
              break;
            case 2:
              reslice <Interp::Cubic> (input, output, transformation, oversampling, oob);
              break;
            case 3:
              reslice <Interp::Sinc> (input, output, transformation, oversampling, oob);
              break;
            default:
              assert (0);
              break;
            }
          }

      protected:
        int interp_type;
        transform_type transformation;
        vector<uint32_t> oversampling;
        default_type *out_of_bounds_value;
    };
    //! @}
  }
}


#endif
