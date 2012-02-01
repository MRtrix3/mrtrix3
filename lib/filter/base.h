/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt, 15/06/11.

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

#ifndef __filter_base_h__
#define __filter_base_h__

#include <cassert>
#include <vector>
#include "ptr.h"
#include "math/matrix.h"
#include "image/header.h"

namespace MR
{
  namespace Filter
  {

    //! \addtogroup Filters
    // @{

    /*! A base class for all image input-output filters.
     * This class is designed to provide a consistent interface for
     * image-to-image filters that perform operations on data defined
     * using the GenericDataSet interface. This allows all derived filters
     * to work with any DataSet type.
     *
     * Typical usage of a filter:
     * \code
     *
     * Image::Header input_header (argument[0]);
     * Image::Voxel<float> input_voxel (input_header);

     * Image::Header mask_header (input_header);
     * // Set the desired output data type
     * mask_header.set_datatype(DataType::Int8);
     *
     * Filter::MyFilterClass<Image::Voxel<float>, Image::Voxel<int> > my_filter(input_voxel);
     *
     * // Let the Filter define the output dimensions, voxel size, stride and transform
     * mask_header.set_params(my_filter.get_output_params();
     * mask_header.create(argument[1]);
     *
     * Image::Voxel<int> mask_voxel (mask_header);
     * my_filter.execute(input_voxel, mask_voxel);
     *
     * \endcode
     */
    template <class InputSet, class OutputSet> class Base
    {
      public:

        typedef typename OutputSet::value_type value_type;

        Base (InputSet& D) :
          axes_ (D.ndim()),
          transform_ (D.transform()) {
          for (size_t n = 0; n < D.ndim(); ++n) {
            axes_[n].dim = D.dim (n);
            axes_[n].vox = D.vox (n);
            axes_[n].stride = D.stride (n);
          }
        }

        virtual ~Base () {}

        virtual void execute (OutputSet& output) = 0;

        Image::Info get_output_params() {
          Image::Info output_prototype;
          output_prototype.set_ndim (axes_.size());
          for (size_t n = 0; n < axes_.size(); ++n) {
            output_prototype.dim(n) = axes_[n].dim;
            output_prototype.vox(n) = axes_[n].vox;
            output_prototype.stride(n) = axes_[n].stride;
          }
          output_prototype.transform() = transform_;
          return output_prototype;
        }

      protected:
        void set_output_ndim (size_t ndim) {
          axes_.resize(ndim);
        }
        void set_output_dim (size_t axis, int dim) {
          axes_[axis].dim = dim;
        }
        void set_output_vox (size_t axis, float vox) {
          axes_[axis].vox = vox;
        }
        void set_output_stride (size_t axis, ssize_t stride) {
          axes_[axis].stride = stride;
        }

      private:
        std::vector<Image::Axis> axes_;
        Math::Matrix<float> transform_;
    };

    //! @}
  }
}

#endif

