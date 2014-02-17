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


#ifndef __image_adapter_voxel_h__
#define __image_adapter_voxel_h__

#include "datatype.h"
#include "math/matrix.h"
#include "image/value.h"
#include "image/position.h"

namespace MR
{
  namespace Image
  {
    class Info;
    class Header;

    namespace Adapter
    {

      template <class VoxelType> 
        class Voxel {
          public:
            Voxel (const VoxelType& parent) :
              parent_vox (parent) {
              }

            typedef typename VoxelType::value_type value_type;
            typedef Voxel voxel_type;

            template <class U> 
              const Voxel& operator= (const U& V) {
                return parent_vox = V;
              }

            const Image::Info& info () const {
              return parent_vox.info();
            }

            const Image::Info& buffer () const {
              return parent_vox.buffer();
            }

            DataType datatype () const {
              return parent_vox.datatype();
            }
            const Math::Matrix<float>& transform () const {
              return parent_vox.transform();
            }

            ssize_t stride (size_t axis) const {
              return parent_vox.stride (axis);
            }
            size_t  ndim () const {
              return parent_vox.ndim();
            }
            ssize_t dim (size_t axis) const {
              return parent_vox.dim (axis);
            }
            float   vox (size_t axis) const {
              return parent_vox.vox (axis);
            }
            const std::string& name () const {
              return parent_vox.name();
            }

            void reset () {
              parent_vox.reset();
            }

            ssize_t operator[] (size_t axis) const {
              return get_pos (axis);
            }
            Image::Position<Voxel> operator[] (size_t axis) {
              return Image::Position<Voxel> (*this, axis);
            }

            value_type value () const {
              return get_value ();
            }
            Image::Value<Voxel> value () {
              return Image::Value<Voxel> (*this);
            }


            friend std::ostream& operator<< (std::ostream& stream, const Voxel& V) {
              stream << "voxel for image \"" << V.name() << "\", datatype " << V.datatype().specifier() << ", position [ ";
              for (size_t n = 0; n < V.ndim(); ++n) 
                stream << V[n] << " ";
              stream << "], value = " << V.value();
              return stream;
            }

          protected:
            VoxelType parent_vox;

            value_type get_value () const {
              return parent_vox.value();
            }

            void set_value (value_type val) {
              parent_vox.value() = val;
            }

            ssize_t get_pos (size_t axis) const {
              return parent_vox[axis];
            }
            void set_pos (size_t axis, ssize_t position) {
              parent_vox[axis] = position;
            }
            void move_pos (size_t axis, ssize_t increment) {
              parent_vox[axis] += increment;
            }

            friend class Image::Position<Voxel>;
            friend class Image::Value<Voxel>;
        };


    }
  }
}

#endif





