/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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
