/*
   Copyright 2010 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 15/10/10.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

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
        using typename Image::Adapter::Voxel<VoxelType>::parent_vox;

        Bootstrap (const VoxelType& Image, const Functor& functor) :
          Image::Adapter::Voxel<VoxelType> (Image),
          func (functor),
          next_voxel (NULL),
          last_voxel (NULL) {
            assert (ndim() == 4);
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




