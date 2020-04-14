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
    // TODO Rename to Convolution?
    namespace Kernel
    {


      // TODO More advanced kernel class?
      // Kernel extent may be different for different axes

      using kernel_type = vector<default_type>;


      template <class AdapterType, class ImageType>
      class Base : public Adapter::Base<AdapterType, ImageType>
      { MEMALIGN(Base<AdapterType, ImageType>)
        public:
          using base_type = Adapter::Base<AdapterType, ImageType>;
          using value_type = typename ImageType::value_type;
          using data_type = Eigen::Matrix<value_type, Eigen::Dynamic, 1>;

          Base (const ImageType& parent) :
              base_type (parent) { }

          template <class Cont>
          Base (const ImageType& parent, const Cont& kernel_sizes) :
              base_type (parent)
          {
            configure_kernel (kernel_sizes);
          }


        protected:
          using base_type::parent_;
          data_type data;
          std::array<size_t, 3> kernel_halfwidths;

          void load_data()
          {
            assert (data.size());
            const std::array<ssize_t, 3> pos ({parent_.index(0), parent_.index(1), parent_.index(2)});
            size_t counter = 0;
            for (parent_.index(2) = pos[2] - kernel_halfwidths[2]; parent_.index(2) <= pos[2] + kernel_halfwidths[2]; ++parent_.index(2)) {
              for (parent_.index(1) = pos[1] - kernel_halfwidths[1]; parent_.index(1) <= pos[1] + kernel_halfwidths[1]; ++parent_.index(1)) {
                for (parent_.index(0) = pos[0] - kernel_halfwidths[0]; parent_.index(0) <= pos[0] + kernel_halfwidths[0]; ++parent_.index(0))
                  data[counter++] = parent_.value();
              }
            }
            for (size_t i = 0; i != 3; ++i)
              parent_.index(i) = pos[i];
          }

          template <class Cont>
          void configure_kernel (const Cont kernel_sizes)
          {
            assert (kernel_sizes.size() == 3);
            for (size_t axis = 0; axis != 3; ++axis) {
              assert (kernel_sizes[axis] % 2);
              kernel_halfwidths[axis] = kernel_sizes[axis]-1 / 2;
            }
            data = data_type::Zero (kernel_sizes[0] * kernel_sizes[1] * kernel_sizes[2]);
          }

          template <>
          void configure_kernel (const size_t kernel_size)
          {
            std::array<size_t, 3> kernel_sizes { kernel_size, kernel_size, kernel_size };
            configure_kernel (kernel_sizes);
          }

      };


      template <class ImageType>
      class Single : public Kernel::Base<Single<ImageType>, ImageType>
      { MEMALIGN(Single<ImageType>)
        public:
          using base_type = Kernel::Base<Single<ImageType>, ImageType>;
          using value_type = typename base_type::value_type;
          using data_type = typename base_type::data_type;

          Single (const ImageType& parent) :
              base_type (parent) { }

          Single (const ImageType& parent, const kernel_type& k) :
              base_type (parent, k.size()),
              kernel (data_type::Zero (k.size()))
          {
            for (size_t i = 0; i != k.size(); ++i)
              kernel[i] = static_cast<value_type>(k[i]);
          }

          value_type value()
          {
            assert (kernel.size() == base_type::data.size());
            base_type::load_data();
            return kernel.dot (base_type::data);
          }

          void set_kernel (const kernel_type& k)
          {
            // No resizing permitted within this version of the function
            assert (k.size() == base_type::data.size());
            base_type::configure_kernel (k.size());
            kernel = data_type::Zero (k.size());
            for (size_t i = 0; i != k.size(); ++i)
              kernel[i] = static_cast<value_type>(k[i]);
          }

          template <class Cont>
          void set_kernel (const kernel_type& k, const Cont& kernel_sizes)
          {
            base_type::configure_kernel (kernel_sizes);
            set_kernel (k);
          }

        private:
          data_type kernel;

      };



      // Class that will wrap management of triplets of filters
      // Interface should be identical to Gradient3D and Laplacian3D
      // TODO Getting this to work will also be predicated on getting rid of the idea of returning
      //   a 3-vector
      template <class ImageType>
      class Triplet : public Kernel::Base<Triplet<ImageType>, ImageType>
      { MEMALIGN(Triplet<ImageType>)
        public:
          using base_type = Base<Triplet<ImageType>, ImageType>;
          using value_type = typename base_type::value_type;
          using data_type = typename base_type::data_type;

          Triplet (const ImageType& parent, const std::array<kernel_type, 3>& k) :
              base_type (parent, k[0].size()),
              kernels ({ data_type::Zero (k[0].size()),
                         data_type::Zero (k[1].size()),
                         data_type::Zero (k[2].size()) }),
              kernel_index (0),
              dirty (true)
          {
            const size_t kernel_size = k[0].size();
            assert (k[1].size() == kernel_size);
            assert (k[2].size() == kernel_size);
            for (size_t i = 0; i != kernel_size; ++i) {
              kernels[0][i] = static_cast<value_type>(k[0][i]);
              kernels[1][i] = static_cast<value_type>(k[1][i]);
              kernels[2][i] = static_cast<value_type>(k[2][i]);
            }
          }

          FORCE_INLINE size_t ndim () const { return parent_.ndim() + 1; }
          FORCE_INLINE ssize_t size (size_t axis) const { if (axis < 3) return parent_.size (axis); if (axis == 3) return 3; return parent_.size (axis-1); }
          FORCE_INLINE default_type spacing (size_t axis) const { if (axis < 3) return parent_.spacing (axis); if (axis == 3) return NaN; return parent_.spacing (axis-1); }
          FORCE_INLINE ssize_t stride (size_t axis) const { if (axis < 3) return 3*parent_.stride (axis); if (axis == 3) return 1; return 3*parent_.stride (axis-1); }

          FORCE_INLINE ssize_t get_index (size_t axis) const
          {
            if (axis < 3)
              return parent_.index (axis);
            if (axis == 3)
              return kernel_index;
            return parent_.index (axis-1);
          }
          FORCE_INLINE void move_index (size_t axis, ssize_t increment)
          {
            if (axis == 3) {
              kernel_index += increment;
              return;
            }
            dirty = true;
            if (axis < 3)
              parent_.index (axis) += increment;
            else
              parent_.index (axis-1) += increment;
          }

          value_type value()
          {
            assert (kernel_index >= 0 && kernel_index < 3);
            if (dirty) {
              base_type::load_data();
              dirty = false;
            }
            return kernels[kernel_index].dot (base_type::data);
          }

        protected:
          using base_type::parent_;
          std::array<data_type, 3> kernels;
          ssize_t kernel_index;
          bool dirty;

      };



      template <class ImageType>
      class TripletNorm : public Kernel::Base<TripletNorm<ImageType>, ImageType>
      { MEMALIGN(TripletNorm<ImageType>)
        public:
          using base_type = Kernel::Base<TripletNorm<ImageType>, ImageType>;
          using value_type = typename base_type::value_type;
          using data_type = typename base_type::data_type;

          TripletNorm (const ImageType& parent, const std::array<kernel_type, 3>& k) :
              base_type (parent, k[0].size()),
              kernels ({ data_type::Zero (k[0].size()),
                         data_type::Zero (k[1].size()),
                         data_type::Zero (k[2].size()) })
          {
            const size_t kernel_size = k[0].size();
            assert (k[1].size() == kernel_size);
            assert (k[2].size() == kernel_size);
            for (size_t i = 0; i != kernel_size; ++i) {
              kernels[0][i] = static_cast<value_type>(k[0][i]);
              kernels[1][i] = static_cast<value_type>(k[1][i]);
              kernels[2][i] = static_cast<value_type>(k[2][i]);
            }
          }

          value_type value()
          {
            base_type::load_data();
            return std::sqrt (Math::pow2 (kernels[0].dot (base_type::data)) +
                              Math::pow2 (kernels[1].dot (base_type::data)) +
                              Math::pow2 (kernels[2].dot (base_type::data)));
          }

        private:
          std::array<data_type, 3> kernels;

      };



    }
  }
}

#endif





