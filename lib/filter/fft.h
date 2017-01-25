/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __image_filter_fft_h__
#define __image_filter_fft_h__

#include <complex>

#include <unsupported/Eigen/FFT>

#include "datatype.h"
#include "memory.h"
#include "image.h"
#include "algo/copy.h"
#include "algo/threaded_copy.h"
#include "filter/base.h"

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
          for (size_t axis = 0; axis != std::min<size_t> (size_t(3), in.ndim()); ++axis)
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

            auto temp = Image<cdouble>::scratch (*this);
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
            FFTKernel (const ComplexImageType& voxel, const size_t FFT_axis, const bool inverse_FFT) :
                vox (voxel),
                data_in (vox.size (FFT_axis)),
                data_out (data_in.size()),
                axis (FFT_axis),
                inverse (inverse_FFT) { }

            void operator () (const Iterator& pos) {
              assign_pos_of (pos).to (vox);
              for (vox.index(axis) = 0; vox.index(axis) < vox.size(axis); ++vox.index(axis))
                data_in[vox.index(axis)] = cdouble (vox.value());
              if (inverse)
                fft.inv (data_out, data_in);
              else
                fft.fwd (data_out, data_in);
              for (vox.index(axis) = 0; vox.index(axis) < vox.size(axis); ++vox.index(axis))
                vox.value() = typename ComplexImageType::value_type (data_out[vox.index(axis)]);
            }

          protected:
            ComplexImageType vox;
            Eigen::Matrix<cdouble, Eigen::Dynamic, 1> data_in, data_out;
            Eigen::FFT<double> fft;
            size_t axis;
            bool inverse;
        };

    };



    template <class ImageType>
      void fft (ImageType&& vox, const size_t axis, const bool inverse = false) {
        auto axes = Stride::order (vox);
        for (size_t n = 0; n < axes.size(); ++n)
          if (axis == axes[n])
            axes.erase (axes.begin() + n);

        struct Kernel {
          Kernel (const ImageType& v, size_t axis, bool inverse) :
            data_in (v.size (axis)), data_out (data_in.size()), axis (axis), inverse (inverse) { }

          void operator ()(ImageType& v) {
            for (auto l = Loop (axis, axis+1) (v); l; ++l)
              data_in[v[axis]] = cdouble (v.value());
            if (inverse)
              fft.inv (data_out, data_in);
            else
              fft.fwd (data_out, data_in);
            for (auto l = Loop (axis, axis+1) (v); l; ++l)
              v.value() = typename std::remove_reference<ImageType>::type::value_type (data_out[v[axis]]);
          }
          Eigen::FFT<double> fft;
          Eigen::Matrix<cdouble, Eigen::Dynamic, 1> data_in, data_out;
          const size_t axis;
          const bool inverse;
        } kernel (vox, axis, inverse);

        ThreadedLoop ("performing in-place FFT", vox, axes)
          .run (kernel, vox);
      }




  }
}


#endif
