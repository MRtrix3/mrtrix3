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


#ifndef __image_adapter_replicate_h__
#define __image_adapter_replicate_h__

#include "math/matrix.h"
#include "image/info.h"
#include "image/position.h"
#include "image/voxel.h"
#include "image/adapter/voxel.h"

namespace MR
{
  namespace Image
  {
    namespace Adapter {

    template <class VoxelType>
      class Replicate : public Voxel<VoxelType>
    {
      public:
        typedef typename VoxelType::value_type value_type;

        using Voxel<VoxelType>::name;
        using Voxel<VoxelType>::ndim;
        using Voxel<VoxelType>::vox;

        template <class InfoType>
          Replicate (VoxelType& original, const InfoType& replication_template) :
            Voxel<VoxelType> (original),
            info_ (replication_template),
            pos_ (ndim(), 0)
          {
            for (size_t n = 0; n < std::min (parent_vox.ndim(), info_.ndim()); ++n) {
              if (parent_vox.dim(n) > 1 && parent_vox.dim(n) != info_.dim(n))
                throw Exception ("cannot replicate over non-singleton dimensions");
            }
          }

        const Image::Info& info() const {
          return info_;
        }

        size_t ndim () const {
          return info_.ndim();
        }
        ssize_t dim (size_t axis) const {
          return info_.dim (axis);
        }
        float vox (size_t axis) const {
          return info_.vox(axis);
        }
        ssize_t stride (size_t axis) const {
          return axis < parent_vox.ndim() ? parent_vox.stride (axis) : 0;
        }

        Position<Replicate<VoxelType> > operator[] (size_t axis) {
          return (Position<Replicate<VoxelType> > (*this, axis));
        }

      protected:
        using Voxel<VoxelType>::parent_vox;
        Image::Info info_;
        std::vector<ssize_t> pos_;

        ssize_t get_pos (size_t axis) const {
          return pos_[axis];
        }
        void set_pos (size_t axis, ssize_t position) {
          pos_[axis] = position;
          if (axis < parent_vox.ndim())
            if (parent_vox.dim(axis) > 1)
              parent_vox[axis] = position;
        }
        void move_pos (size_t axis, ssize_t increment) {
          pos_[axis] += increment;
          if (axis < parent_vox.ndim())
            if (parent_vox.dim(axis) > 1)
              parent_vox[axis] += increment;
        }

        friend class Position<Replicate<VoxelType> >;
    };

    }
  }
}

#endif



