/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __image_adapter_subset_h__
#define __image_adapter_subset_h__

#include "math/matrix.h"
#include "image/info.h"
#include "image/position.h"
#include "image/value.h"
#include "image/voxel.h"
#include "image/adapter/voxel.h"

namespace MR
{
  namespace Image
  {
    namespace Adapter {

    template <class VoxelType>
      class Subset : public Voxel<VoxelType>
    {
      public:
        typedef typename VoxelType::value_type value_type;

        using Voxel<VoxelType>::name;
        using Voxel<VoxelType>::ndim;
        using Voxel<VoxelType>::vox;

        template <class VectorType>
          Subset (const VoxelType& original, const VectorType& from, const VectorType& dimensions) :
            Voxel<VoxelType> (original),
            from_ (ndim()),
            info_ (original)
          {
            for (size_t n = 0; n < ndim(); ++n) {
              assert (ssize_t (from[n] + dimensions[n]) <= original.dim(n));
              from_[n] = from[n];
              info_.dim(n) = dimensions[n];
            }

            for (size_t j = 0; j < 3; ++j)
              for (size_t i = 0; i < 3; ++i)
                info_.transform()(i,3) += from[j] * vox(j) * info_.transform()(i,j);
          }

        const Image::Info& info() const {
          return info_;
        }

        void reset () {
          for (size_t n = 0; n < ndim(); ++n)
            set_pos (n, 0);
        }

        ssize_t dim (size_t axis) const {
          return info_.dim (axis);
        }
        const Math::Matrix<float>& transform() const { 
          return info_.transform();
        }

        Value<Subset<VoxelType> > value () {
          return (Value<Subset<VoxelType> > (*this));
        }
        Position<Subset<VoxelType> > operator[] (size_t axis) {
          return (Position<Subset<VoxelType> > (*this, axis));
        }

      protected:
        using Voxel<VoxelType>::parent_vox;
        std::vector<ssize_t> from_;
        Image::Info info_;

        value_type get_value () const {
          return parent_vox.value();
        }
        void set_value (value_type val) {
          parent_vox.value() = val;
        }

        ssize_t get_pos (size_t axis) const {
          return parent_vox[axis]-from_[axis];
        }
        void set_pos (size_t axis, ssize_t position) {
          parent_vox[axis] = position + from_[axis];
        }
        void move_pos (size_t axis, ssize_t increment) {
          parent_vox[axis] += increment;
        }

        friend class Value<Subset<VoxelType> >;
        friend class Position<Subset<VoxelType> >;
    };

    }
  }
}

#endif


