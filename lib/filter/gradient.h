/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
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
    class Gradient : public Base { MEMALIGN(Gradient)
      public:
        template <class HeaderType>
        Gradient (const HeaderType& in, const bool magnitude = false) :
            Base (in),
            smoother (in),
            wrt_scanner (true),
            magnitude (magnitude),
            stdev (1, 0)
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
          DEBUG ("creating gradient filter");
        }

        void compute_wrt_scanner (bool do_wrt_scanner) {
          wrt_scanner = do_wrt_scanner;
        }

        void set_stdev (const vector<default_type>& stdevs) {
          stdev = stdevs;
        }


        template <class InputImageType, class OutputImageType>
        void operator() (InputImageType& in, OutputImageType& out)
        {
          if (magnitude) {
            Gradient full_gradient (in, false);
            full_gradient.set_stdev (stdev);
            full_gradient.compute_wrt_scanner (wrt_scanner);
            full_gradient.set_message (message);
            auto temp = Image<float>::scratch (full_gradient, "full 3D gradient image");
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
          smoother.set_stdev (stdev);
          auto smoothed = Image<float>::scratch (smoother);
          if (message.size())
            smoother.set_message ("applying smoothing prior to calculating gradient");
          threaded_copy (in, smoothed);
          smoother (smoothed);

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
              Transform transform (in);
              for (auto l = Loop(0,3) (out); l; ++l)
                out.row(3) = transform.image2scanner.linear() * Eigen::Vector3 (out.row(3));
            }
          }
        }

      protected:
        Filter::Smooth smoother;
        bool wrt_scanner;
        const bool magnitude;
        vector<default_type> stdev;
    };
    //! @}
  }
}

#endif
