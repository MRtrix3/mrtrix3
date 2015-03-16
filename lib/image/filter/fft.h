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

#include <complex>

#include "datatype.h"
#include "image/buffer_scratch.h"
#include "image/copy.h"
#include "image/info.h"
#include "image/nav.h"
#include "image/threaded_copy.h"
#include "image/filter/base.h"
#include "math/fft.h"

namespace MR
{
  namespace Image
  {
    namespace Filter
    {



      /** \addtogroup Filters
        @{ */

      //! a filter to perform an FFT on an image
      /*!
       * Typical usage:
       * \code
       * Buffer<complex_value_type> input_data (argument[0]);
       * auto nput_voxel = input_data.voxel();
       *
       * Filter::FFT fft (input_data);
       * Header header (input_data);
       * header.info() = fft.info();
       *
       * Buffer<complex_value_type> output_data (header, argument[1]);
       * auto output_voxel = output_data.voxel();
       * fft (input_voxel, output_voxel);
       *
       * \endcode
       */
      class FFT : public Base
      {
        public:

          template <class InfoType>
          FFT (const InfoType& in, const bool inverse) :
              Base (in),
              inverse (inverse),
              centre_zero_ (false)
          {
            for (size_t axis = 0; axis != std::min (size_t(3), in.ndim()); ++axis)
              axes_to_process.push_back (axis);
            datatype_ = DataType::CFloat64;
            datatype_.set_byte_order_native();
          }


          void set_axes (const std::vector<int>& in)
          {
            axes_to_process.clear();
            for (std::vector<int>::const_iterator i = in.begin(); i != in.end(); ++i) {
              if (*i < 0)
                throw Exception ("Axis indices for FFT image filter must be positive");
              if (*i >= (int)this->ndim())
                throw Exception ("Axis index " + str(*i) + " for FFT image filter exceeds number of image dimensions (" + str(this->ndim()) + ")");
              axes_to_process.push_back (*i);
            }
          }


          void set_centre_zero (const bool i)
          {
            centre_zero_ = i;
          }


          template <class InputComplexVoxelType, class OutputComplexVoxelType>
          void operator() (InputComplexVoxelType& input, OutputComplexVoxelType& output)
          {

              Ptr<ProgressBar> progress;
              if (message.size())
                progress = new ProgressBar (message, axes_to_process.size() + 2);

              Image::BufferScratch<cdouble> temp_data (info());
              auto temp_voxel = temp_data.voxel();
              Image::copy (input, temp_voxel);
              if (progress) ++(*progress);

              for (std::vector<size_t>::const_iterator axis = axes_to_process.begin(); axis != axes_to_process.end(); ++axis) {
                std::vector<size_t> axes = Stride::order (temp_voxel);
                for (size_t n = 0; n < axes.size(); ++n) {
                  if (axes[n] == *axis) {
                    axes.erase (axes.begin() + n);
                    break;
                  }
                }
                FFTKernel<decltype(temp_voxel)> kernel (temp_voxel, *axis, inverse);
                ThreadedLoop (temp_voxel, axes, 1).run (kernel);
                if (progress) ++(*progress);
              }

              if (centre_zero_) {
                Image::LoopInOrder loop (output);
                for (auto l = loop (output); l; ++l) {
                  Image::Nav::set_pos (temp_voxel, output);
                  for (std::vector<size_t>::const_iterator flip_axis = axes_to_process.begin(); flip_axis != axes_to_process.end(); ++flip_axis)
                    temp_voxel[*flip_axis] = (temp_voxel[*flip_axis] >= (temp_voxel.dim (*flip_axis) / 2)) ?
                                             (temp_voxel[*flip_axis] - (temp_voxel.dim (*flip_axis) / 2)) :
                                             (temp_voxel[*flip_axis] + (temp_voxel.dim (*flip_axis) / 2));
                  output.value() = temp_voxel.value();
                }
              } else {
                Image::copy (temp_voxel, output);
              }
          }


        protected:

          const bool inverse;
          std::vector<size_t> axes_to_process;
          bool centre_zero_;

          template <class ComplexVoxelType>
          class FFTKernel {
            public:
              FFTKernel (ComplexVoxelType& voxel, size_t FFT_axis, bool inverse_FFT) :
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

      };



      template <class VoxelType>
        void fft (VoxelType&& vox, size_t axis, bool inverse = false) {
          auto axes = Image::Stride::order (vox);
          for (size_t n = 0; n < axes.size(); ++n) 
            if (axis == axes[n])
              axes.erase (axes.begin() + n);

          struct Kernel {
            Kernel (const VoxelType& v, size_t axis, bool inverse) :
              data (v.dim (axis)), axis (axis), inverse (inverse) { }

            void operator ()(VoxelType& v) {
              for (auto l = Image::Loop (axis, axis+1) (v); l; ++l) 
                data[v[axis]] = cdouble (v.value());
              fft (data, inverse);
              for (auto l = Image::Loop (axis, axis+1) (v); l; ++l) 
                v.value() = typename std::remove_reference<VoxelType>::type::value_type (data[v[axis]]);
            }
            Math::FFT fft; 
            std::vector<cdouble> data;
            const size_t axis;
            const bool inverse;
          } kernel (vox, axis, inverse);

          Image::ThreadedLoop ("performing in-place FFT...", vox, axes)
            .run (kernel, vox);
        }




    }
  }
}


#endif
