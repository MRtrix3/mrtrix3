/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#ifndef __adapter_vector2axis_h__
#define __adapter_vector2axis_h__


namespace MR
{
  namespace Adapter
  {



    // TODO Need an adapter that takes an Eigen::Vector (or anything similar) from the
    //   output of a functor, and writes those data along a particular axis of an image
    // e.g. Vector2ImageAxis
    //      Scalar2Vector
    // Wraps the functor itself
    // This would be used in instances where all components of the filter output are to be retained
    template <class Functor>
    class Vector2Axis : public Functor
    { MEMALIGN(Vector2Axis)
      public:
        using Functor::Functor;

        template <class ImageTypeIn, class ImageTypeOut>
        void operator() (ImageTypeIn& in, ImageTypeOut& out)
        {
          assert (out.ndim() = in.ndim() + 1);
          const size_t out_axis = out.ndim() - 1;
          auto result = Functor::operator() (in);
          assert (result.size() == out.size(out_axis));
          for (size_t i = 0; i != result.size(); ++i) {
            out.index(out_axis) = i;
            out.value() = result[i];
          }
        }
    };



  }
}

#endif





