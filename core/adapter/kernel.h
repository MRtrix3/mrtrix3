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

#include "types.h"
#include "adapter/base.h"
#include "filter/kernels.h"
#include "math/math.h"

namespace MR
{
  namespace Adapter
  {
    // TODO Rename to Convolution?
    namespace Kernel
    {


      // TODO More advanced kernel class?
      // Kernel extent may be different for different axes

      //using kernel_type = vector<default_type>;
      //using kernel_type = MR::Filter::Kernels::kernel_type;
      //using kernel_triplet = MR::Filter::Kernels::kernel_triplet;


      template <class AdapterType, class ImageType>
      class Base : public Adapter::Base<AdapterType, ImageType>
      { MEMALIGN(Base<AdapterType, ImageType>)
        public:
          using base_type = Adapter::Base<AdapterType, ImageType>;
          using value_type = typename ImageType::value_type;
          using data_type = Eigen::Matrix<value_type, Eigen::Dynamic, 1>;

          Base (const ImageType& parent) :
              base_type (parent) { }

          Base (const ImageType& parent, const Filter::Kernels::kernel_type& kernel) :
              base_type (parent)
          {
            configure_kernel (kernel);
          }


        protected:
          using base_type::parent_;
          data_type data;
          std::array<size_t, 3> kernel_halfwidths;


          // TODO Ideally would like to set the kernel up such that the
          //   looping over the three spatial axes is done in the same order
          //   as the relative image strides, and the data vector can still be
          //   filled in a linear fashion
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

          void configure_kernel (const Filter::Kernels::kernel_type& k)
          {
            for (size_t axis = 0; axis != 3; ++axis)
              kernel_halfwidths[axis] = k.halfsize (axis);
            data = data_type::Zero (k.size());
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

          Single (const ImageType& parent, const Filter::Kernels::kernel_type& k) :
              base_type (parent, k),
              kernel (k.cast<value_type> ()) { }

          value_type value()
          {
            assert (kernel.size() == base_type::data.size());
            base_type::load_data();
            return kernel.dot (base_type::data);
          }

          void set_kernel (const Filter::Kernels::kernel_type& k)
          {
            base_type::configure_kernel (k);
            kernel = k.cast<value_type> ();
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

          Triplet (const ImageType& parent, const Filter::Kernels::kernel_triplet& k) :
              base_type (parent, k[0]),
              kernels ({ k[0].cast<value_type> (),
                         k[1].cast<value_type> (),
                         k[2].cast<value_type> () }),
              kernel_index (0),
              dirty (true)
          {
            assert (k[1].size() == k[0].size());
            assert (k[2].size() == k[0].size());
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

          TripletNorm (const ImageType& parent, const Filter::Kernels::kernel_triplet& k) :
              base_type (parent, k[0]),
              kernels ({ k[0].cast<value_type> (),
                         k[1].cast<value_type> (),
                         k[2].cast<value_type> () })
          {
            assert (k[1].size() == k[0].size());
            assert (k[2].size() == k[0].size());
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





