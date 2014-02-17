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


#ifndef __image_adapter_permute_axes_h__
#define __image_adapter_permute_axes_h__

#include "image/adapter/voxel.h"
#include "image/position.h"

namespace MR
{
  namespace Image
  {
    namespace Adapter
    {

      template <class VoxelType> 
        class PermuteAxes : public Voxel<VoxelType>
      {
        public:
          using Voxel<VoxelType>::dim;
          using Voxel<VoxelType>::parent_vox;
          typedef typename VoxelType::value_type value_type;

          PermuteAxes (const VoxelType& original, const std::vector<int>& axes) :
            Voxel<VoxelType> (original), 
            axes_ (axes) {
              for (int i = 0; i < static_cast<int> (parent_vox.ndim()); ++i) {
                for (size_t a = 0; a < axes_.size(); ++a)
                  if (axes_[a] == i)
                    goto next_axis;
                if (parent_vox.dim (i) != 1)
                  throw Exception ("ommitted axis \"" + str (i) + "\" has dimension greater than 1");
next_axis:
                continue;
              }
            }

          Image::Info info () const { 
            return Image::Info (*this);
          };

          size_t ndim () const {
            return axes_.size();
          }
          int dim (size_t axis) const {
            return axes_[axis] < 0 ? 1 : parent_vox.dim (axes_[axis]);
          }
          float vox (size_t axis) const {
            return axes_[axis] < 0 ? NAN : parent_vox.vox (axes_[axis]);
          }
          ssize_t stride (size_t axis) const {
            return axes_[axis] < 0 ? 0 : parent_vox.stride (axes_[axis]);
          }

          void reset () {
            parent_vox.reset();
          }

          Position<PermuteAxes<VoxelType> > operator[] (size_t axis) {
            return Position<PermuteAxes<VoxelType> > (*this, axis);
          }

        private:
          const std::vector<int> axes_;

          ssize_t get_pos (size_t axis) {
            return axes_[axis] < 0 ? 0 : parent_vox[axes_[axis]];
          }
          void set_pos (size_t axis, ssize_t position) {
            parent_vox[axes_[axis]] = position;
          }
          void move_pos (size_t axis, ssize_t increment) {
            parent_vox[axes_[axis]] += increment;
          }

          friend class Position<PermuteAxes<VoxelType> >;
          friend class Value<PermuteAxes<VoxelType> >;
      };

    }
  }
}

#endif


