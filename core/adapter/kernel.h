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

#ifndef __algo_adapter_kernel_h__
#define __algo_adapter_kernel_h__

#include <array>

#include "math/math.h"
#include "types.h"
#include "adapter/base.h"

namespace MR
{
  namespace Adapter
  {
    namespace Kernel
    {

      using kernel_type = vector<default_type>;


      template <class EdgeAdapterType>
      class Single : public Base<Single<EdgeAdapterType>, EdgeAdapterType>
      { MEMALIGN(Single<EdgeAdapterType>)
        public:
          using base_type = Base<Single<EdgeAdapterType>, EdgeAdapterType>;
          using value_type = typename EdgeAdapterType::value_type;
          using data_type = Eigen::Matrix<value_type, Eigen::Dynamic, 1>;
          using kernel_type = vector<default_type>;

          using base_type::index;

          Single (EdgeAdapterType& parent, const kernel_type& k) :
              base_type (parent),
              kernel (data_type::Zero (k.size())),
              data (data_type::Zero (k.size()))
          {
            const default_type kernel_width = std::cbrt (k.size());
            const size_t kernel_width_int (static_cast<size_t>(round(kernel_width)));
            assert (Math::pow3 (kernel_width_int) == k.size());
            assert (kernel_width_int % 2);
            kernel_halfwidth = (kernel_width_int-1) / 2;
            for (size_t i = 0; i != k.size(); ++i)
              kernel[i] = static_cast<value_type>(k[i]);
          }

          value_type value()
          {
            const std::array<ssize_t, 3> pos ({index(0), index(1), index(2)});
            size_t counter = 0;
            for (index(2) = pos[2] - kernel_halfwidth; index(2) <= pos[2] + kernel_halfwidth; ++index(2)) {
              for (index(1) = pos[1] - kernel_halfwidth; index(1) <= pos[1] + kernel_halfwidth; ++index(1)) {
                for (index(0) = pos[0] - kernel_halfwidth; index(0) <= pos[0] + kernel_halfwidth; ++index(0))
                  data[counter++] = base_type::value();
              }
            }
            for (size_t i = 0; i != 3; ++i)
              index(i) = pos[i];
            return kernel.dot (data);
          }

        private:
          data_type kernel;
          data_type data;
          ssize_t kernel_halfwidth;

      };



      // Class that will wrap management of triplets of filters
      // Interface should be identical to Gradient3D and Laplacian3D
      template <class EdgeAdapterType>
      class Triplet : public Base<Triplet<EdgeAdapterType>, EdgeAdapterType>
      { MEMALIGN(Triplet<EdgeAdapterType>)
        public:

          using base_type = Base<Triplet<EdgeAdapterType>, EdgeAdapterType>;
          using value_type = typename EdgeAdapterType::value_type;
          using return_type = Eigen::Matrix<value_type,3,1>;

          Triplet (EdgeAdapterType& parent, const kernel_type& x, const kernel_type& y, const kernel_type& z) :
              base_type (parent),
              kernels ({ Single<EdgeAdapterType> (parent, x),
                         Single<EdgeAdapterType> (parent, y),
                         Single<EdgeAdapterType> (parent, z) }) { }

          return_type value()
          {
            return_type result;
            for (size_t axis = 0; axis != 3; ++axis) {
              assign_pos_of (*this, 0, 3).to (kernels[axis]);
              result[axis] = kernels[axis].value();
            }
            return result;
          }

        protected:
          std::array<Single<EdgeAdapterType>, 3> kernels;
      };


    }
  }
}

#endif





