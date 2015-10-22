/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 17/02/12.

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

#ifndef __image_filter_gradient_h__
#define __image_filter_gradient_h__

#include "memory.h"
#include "image.h"
#include "transform.h"
#include "algo/loop.h"
#include "algo/threaded_copy.h"
#include "adapter/gradient1D.h"
#include "filter/base.h"
#include "filter/smooth.h"

namespace MR
{
  namespace Filter
  {
    /** \addtogroup Filters
    @{ */

    /*! Compute the image gradients of a 3D or 4D image.
     *
     * Typical usage:
     * \code
     * auto input = Image<float>::open (argument[0]);
     * Filter::Gradient gradient_filter (input);
     * auto output = Image<float>::create (argument[1], gradient_filter);
     * gradient_filter (input, output);
     * \endcode
     */
    class Gradient : public Base
    {
      public:
        Gradient (const Header& in, const bool magnitude = false) :
            Base (in),
            smoother (in),
            wrt_scanner (true),
            magnitude (magnitude)
        {
          if (in.ndim() == 4) {
            if (!magnitude) {
              axes_.resize (5);
              axes_[3].size = 3;
              axes_[4].size = in.size(3);
              axes_[0].stride = 2;
              axes_[1].stride = 3;
              axes_[2].stride = 4;
              axes_[3].stride = 1;
              axes_[4].stride = 5;
            }
          } else if (in.ndim() == 3) {
            if (!magnitude) {
              axes_.resize (4);
              axes_[3].size = 3;
              axes_[0].stride = 2;
              axes_[1].stride = 3;
              axes_[2].stride = 4;
              axes_[3].stride = 1;
            }
          } else {
            throw Exception("input image must be 3D or 4D");
          }
          datatype() = DataType::Float32;
        }

        void compute_wrt_scanner (bool do_wrt_scanner) {
          if (do_wrt_scanner && magnitude)
            WARN ("For a gradient magnitude image, setting gradient to scanner axes has no effect");
          wrt_scanner = do_wrt_scanner;
        }

        void set_stdev (const std::vector<default_type>& stdevs)
        {
          smoother.set_stdev (stdevs);
        }


        template <class InputImageType, class OutputImageType>
        void operator() (InputImageType& in, OutputImageType& out)
        {
          if (magnitude) {
            Gradient full_gradient (in.header(), false);
            full_gradient.set_message (message);
            auto temp = Image<float>::scratch (full_gradient.header(), "full 3D gradient image");
            full_gradient (in, temp);
            for (auto l = Loop (out)(out, temp); l; ++l) {
              if (out.ndim() == 4) {
                ssize_t tmp = out.index(3);
                temp.index(4) = tmp;
              }
              float grad_sq = 0.0;
              for (temp.index(3) = 0; temp.index(3) != 3; ++temp.index(3))
                grad_sq += Math::pow2<float> (temp.value());
              out.value() = std::sqrt (grad_sq);
            }
            return;
          }

          auto smoothed = Image<float>::scratch (smoother.header());
          if (message.size())
            smoother.set_message ("applying smoothing prior to calculating gradient... ");
          smoother (in, smoothed);

          const size_t num_volumes = (in.ndim() == 3) ? 1 : in.size(3);

          std::unique_ptr<ProgressBar> progress (message.size() ?  new ProgressBar (message, 3 * num_volumes) : nullptr);

          for (size_t vol = 0; vol < num_volumes; ++vol) {
            if (in.ndim() == 4) {
              smoothed.index(3) = vol;
              out.index(4) = vol;
            }

            Adapter::Gradient1D<decltype(smoothed)> gradient1D (smoothed, 0, wrt_scanner);
            out.index(3) = 0;
            threaded_copy (gradient1D, out, 0, 3, 2);
            if (progress) ++(*progress);
            out.index(3) = 1;
            gradient1D.set_axis (1);
            threaded_copy (gradient1D, out, 0, 3, 2);
            if (progress) ++(*progress);
            out.index(3) = 2;
            gradient1D.set_axis (2);
            threaded_copy (gradient1D, out, 0, 3, 2);
            if (progress) ++(*progress);

            if (wrt_scanner) {
              Transform transform (in.header());
              for (auto l = Loop(0,3) (out); l; ++l)
                out.row(3) = transform.image2scanner.linear().template cast<typename OutputImageType::value_type>() * out.row(3);
            }
          }
        }

      protected:
        Filter::Smooth smoother;
        bool wrt_scanner;
        const bool magnitude;
    };
    //! @}
  }
}

#endif
