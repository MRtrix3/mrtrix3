/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#ifndef __image_filter_fft_h__
#define __image_filter_fft_h__

#include "image/info.h"
#include "image/threaded_copy.h"
#include "math/fft.h"

namespace MR
{
  namespace Image
  {
    namespace Filter
    {

      namespace {

      template <class ComplexVoxelType>
        class __FFTKernel {
          public:
            __FFTKernel (ComplexVoxelType& voxel, size_t FFT_axis, bool inverse_FFT) :
              vox (voxel),
              data (vox.dim (FFT_axis)),
              axis (FFT_axis),
              inverse (inverse_FFT) { }

            void operator () (const Iterator& pos) {
              voxel_assign (vox, pos);
              for (vox[axis] = 0; vox[axis] < vox.dim(axis); ++vox[axis])
                data[vox[axis]] = cdouble (vox.value());
              fft (data, inverse);
              for (vox[axis] = 0; vox[axis] < vox.dim(axis); ++vox[axis])
                vox.value() = typename ComplexVoxelType::value_type (data[vox[axis]]);
            }

          protected:
            ComplexVoxelType vox;
            std::vector<cdouble> data;
            Math::FFT fft;
            size_t axis;
            bool inverse;
        };

      }


      /** \addtogroup Filters
      @{ */

      template <class ComplexVoxelType>
        inline void fft (ComplexVoxelType& vox, size_t axis, 
            bool inverse = false, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) {
          std::vector<size_t> axes = Stride::order (vox, from_axis, to_axis);
          for (size_t n = 0; n < axes.size(); ++n) {
            if (axes[n] == axis) {
              axes.erase (axes.begin() + n);
              break;
            }
          }

          __FFTKernel<ComplexVoxelType> fft_kernel (vox, axis, inverse);
          ThreadedLoop ("performing FFT of \"" + vox.name() + "\" along axis " + str(axis), vox, axes, 1)
            .run (fft_kernel);

        }
      //! @}
    }
  }
}


#endif
