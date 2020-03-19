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

#ifndef __image_filter_norm_wrapper_h__
#define __image_filter_norm_wrapper_h__

#include "adapter/vector2axis.h"
#include "adapter/permute_axes.h"
#include "adapter/subset.h"

namespace MR
{
  namespace Filter
  {



    // TODO This needs to work with both Kernel::Triplet and Gradient3D / Laplace3D
    template <class FilterType>
    class NormWrapper : public Adapter::Scalar2Vector<FilterType>
    { MEMALIGN(NormWrapper)
      public:
        using base_type = Adapter::Scalar2Vector<FilterType>;
        NormWrapper (const FilterType& filter) :
            filter (filter),
            H_scratch (filter)
        {
          assert (filter.ndim() == 3 || filter.ndim() == 4);
          H_scratch.ndim() = 4;
          H_scratch.size(3) = 3;
          H_scratch.datatype() = datatype() = DataType::Float32;
        }



        template <class InputImageType, class OutputImageType>
        void operator() (InputImageType& in, OutputImageType& out)
        {
          assert (in.ndim() == out.ndim());
          check_dimensions (in, *this);
          check_dimensions (in, out);

          if (in.ndim() == 4) {
            vector<size_t> from ({0, 0, 0, 0});
            vector<size_t> to   ({in.size(0), in.size(1), in.size(2), 1});
            vector<size_t> axes ({0, 1, 2});
            for (size_t volume = 0; volume != in.size(3); ++volume) {
              from[3] = volume;
              to[3] = volume + 1;
              Adapter::Subset<InputImageType>  subset_in  (in,  from, to);
              Adapter::Subset<OutputImageType> subset_out (out, from, to);
              Adapter::PermuteAxes<decltype(subset_in )> permute_in  (subset_in,  axes);
              Adapter::PermuteAxes<decltype(subset_out)> permute_out (subset_out, axes);
              (*this) (permute_in, permute_out);
            }
            return;
          }

          assert (in.ndim() == 3);
          auto scratch = Image<float>::scratch (H_scratch, "Scratch 3-vector image pre-norm");
          filter (in, scratch, 3);
          for (auto l = Loop(out) (scratch, out); l; ++l)
            out.value() = scratch.row(3).norm();
        }


      protected:
        Adapter::Vector2Axis<FilterType> filter;
    };



  }
}

#endif
