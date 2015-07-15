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
#include "memory.h"
#include "image.h"
#include "algo/copy.h"
#include "algo/threaded_copy.h"
#include "filter/base.h"
#include "math/fft.h"

namespace MR
{
  namespace Filter
  {

    /** \addtogroup Filters
      @{ */

    //! a filter to perform an FFT on an image
    /*!
     * Typical usage:
     * \code
     * auto input = Image<complex_value_type>::open(argument[0]);
     * Filter::FFT fft (input);
     * auto output = Image::create<complex_value_type> (fft, argument[1]);
     * fft (input, output);
     *
     * \endcode
     */
    class FFT : public Base
    {
      public:

        template <class HeaderType>
        FFT (const HeaderType& in, const bool inverse) :
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


        template <class InputComplexImageType, class OutputComplexImageType>
        void operator() (InputComplexImageType& input, OutputComplexImageType& output)
        {

          std::shared_ptr<ProgressBar> progress (message.size() ? new ProgressBar (message, axes_to_process.size() + 2) : nullptr);

            auto temp = Image<cdouble>::scratch (header());
            copy (input, temp);
            if (progress)
              ++(*progress);

            for (std::vector<size_t>::const_iterator axis = axes_to_process.begin(); axis != axes_to_process.end(); ++axis) {
              std::vector<size_t> axes = Stride::order (temp);
              for (size_t n = 0; n < axes.size(); ++n) {
                if (axes[n] == *axis) {
                  axes.erase (axes.begin() + n);
                  break;
                }
              }
              FFTKernel<decltype(temp)> kernel (temp, *axis, inverse);
              ThreadedLoop (temp, axes, 1).run (kernel);
              if (progress) ++(*progress);
            }

            if (centre_zero_) {
              for (auto l = Loop (output)(output); l; ++l) {
                assign_pos_of (output).to (temp);
                for (std::vector<size_t>::const_iterator flip_axis = axes_to_process.begin(); flip_axis != axes_to_process.end(); ++flip_axis)
                  temp.index(*flip_axis) = (temp.index(*flip_axis) >= (temp.size (*flip_axis) / 2)) ?
                                           (temp.index(*flip_axis) - (temp.size (*flip_axis) / 2)) :
                                           (temp.index(*flip_axis) + (temp.size (*flip_axis) / 2));
                output.value() = temp.value();
              }
            } else {
              copy (temp, output);
            }
        }


      protected:

        const bool inverse;
        std::vector<size_t> axes_to_process;
        bool centre_zero_;

        template <class ComplexImageType>
        class FFTKernel {
          public:
            FFTKernel (ComplexImageType& voxel, size_t FFT_axis, bool inverse_FFT) :
                vox (voxel),
                data (vox.size (FFT_axis)),
                axis (FFT_axis),
                inverse (inverse_FFT) { }

            void operator () (const Iterator& pos) {
              assign_pos_of (pos).to (vox);
              for (vox.index(axis) = 0; vox.index(axis) < vox.size(axis); ++vox.index(axis))
                data[vox.index(axis)] = cdouble (vox.value());
              fft (data, inverse);
              for (vox.index(axis) = 0; vox.index(axis) < vox.size(axis); ++vox.index(axis))
                vox.value() = typename ComplexImageType::value_type (data[vox.index(axis)]);
            }

          protected:
            ComplexImageType vox;
            std::vector<cdouble> data;
            Math::FFT fft;
            size_t axis;
            bool inverse;
        };

    };



    template <class ImageType>
      void fft (ImageType&& vox, size_t axis, bool inverse = false) {
        auto axes = Stride::order (vox);
        for (size_t n = 0; n < axes.size(); ++n)
          if (axis == axes[n])
            axes.erase (axes.begin() + n);

        struct Kernel {
          Kernel (const ImageType& v, size_t axis, bool inverse) :
            data (v.size (axis)), axis (axis), inverse (inverse) { }

          void operator ()(ImageType& v) {
            for (auto l = Loop (axis, axis+1) (v); l; ++l)
              data[v[axis]] = cdouble (v.value());
            fft (data, inverse);
            for (auto l = Loop (axis, axis+1) (v); l; ++l)
              v.value() = typename std::remove_reference<ImageType>::type::value_type (data[v[axis]]);
          }
          Math::FFT fft;
          std::vector<cdouble> data;
          const size_t axis;
          const bool inverse;
        } kernel (vox, axis, inverse);

        ThreadedLoop ("performing in-place FFT...", vox, axes)
          .run (kernel, vox);
      }




  }
}


#endif
