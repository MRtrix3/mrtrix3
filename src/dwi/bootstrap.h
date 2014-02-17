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


#ifndef __dwi_bootstrap_h__
#define __dwi_bootstrap_h__

#include "ptr.h"
#include "image/position.h"
#include "image/adapter/voxel.h"


namespace MR {
  namespace DWI {


    template <class VoxelType, class Functor, size_t NUM_VOX_PER_CHUNK = 256> 
      class Bootstrap : public Image::Adapter::Voxel<VoxelType>
    {
      public:
        typedef typename VoxelType::value_type value_type;
        using Image::Adapter::Voxel<VoxelType>::dim;
        using Image::Adapter::Voxel<VoxelType>::parent_vox;

        Bootstrap (const VoxelType& Image, const Functor& functor) :
          Image::Adapter::Voxel<VoxelType> (Image),
          func (functor),
          next_voxel (NULL),
          last_voxel (NULL) {
            assert (Image::Adapter::Voxel<VoxelType>::ndim() == 4);
          }

        value_type value () { 
          return get_voxel()[parent_vox[3]]; 
        }

        void get_values (value_type* buffer) { 
          if (parent_vox[0] < 0 || parent_vox[0] >= dim(0) ||
              parent_vox[1] < 0 || parent_vox[1] >= dim(1) ||
              parent_vox[2] < 0 || parent_vox[2] >= dim(2)) 
            memset (buffer, 0, dim(3)*sizeof(value_type));
          else
            memcpy (buffer, get_voxel(), dim(3)*sizeof(value_type)); 
        }

        Image::Position<Bootstrap<VoxelType,Functor,NUM_VOX_PER_CHUNK> > operator[] (size_t axis ) {
          return Image::Position<Bootstrap<VoxelType,Functor,NUM_VOX_PER_CHUNK> > (*this, axis);
        } 

        void clear () 
        {
          voxels.clear(); 
          if (voxel_buffer.empty())
            voxel_buffer.push_back (new value_type [NUM_VOX_PER_CHUNK * dim(3)]);
          next_voxel = voxel_buffer[0];
          last_voxel = next_voxel + NUM_VOX_PER_CHUNK * dim(3);
          current_chunk = 0;
        }

      protected:
        Functor func;
        std::map<Point<ssize_t>,value_type*> voxels;
        VecPtr<value_type,true> voxel_buffer;
        value_type* next_voxel;
        value_type* last_voxel;
        size_t current_chunk;

        value_type* allocate_voxel () 
        {
          if (next_voxel == last_voxel) {
            ++current_chunk;
            if (current_chunk >= voxel_buffer.size()) 
              voxel_buffer.push_back (new value_type [NUM_VOX_PER_CHUNK * dim(3)]);
            assert (current_chunk < voxel_buffer.size());
            next_voxel = voxel_buffer.back();
            last_voxel = next_voxel + NUM_VOX_PER_CHUNK * dim(3);
          }
          value_type* retval = next_voxel;
          next_voxel += dim(3);
          return retval;
        }

        value_type* get_voxel ()
        {
          value_type*& data (voxels.insert (std::make_pair (Point<ssize_t> (parent_vox[0], parent_vox[1], parent_vox[2]), (float*)NULL)).first->second);
          if (!data) {
            data = allocate_voxel ();
            ssize_t pos = parent_vox[3];
            for (parent_vox[3] = 0; parent_vox[3] < dim(3); ++parent_vox[3])
              data[parent_vox[3]] = parent_vox.value();
            parent_vox[3] = pos;
            func (data);
          }
          return data;
        }

        friend class Image::Position<Bootstrap<VoxelType,Functor,NUM_VOX_PER_CHUNK> >;
    };

  }
}

#endif




