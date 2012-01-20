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


namespace MR {
  namespace DWI {


    template <class Set, class Functor, size_t NUM_VOX_PER_CHUNK = 256> class Bootstrap 
    {
      public:
        typedef typename Set::value_type value_type;

        Bootstrap (Set& Image, Functor& functor) :
          S (Image),
          func (functor),
          next_voxel (NULL),
          last_voxel (NULL) {
            assert (S.ndim() == 4);
          }

        const std::string& name () const { return S.name(); }
        size_t ndim () const { return 4; }
        int dim (size_t axis) const { return S.dim (axis); }
        float vox (size_t axis) const { return S.vox (axis); }
        const Math::Matrix<float>& transform () const { return S.transform(); }
        ssize_t stride (size_t axis) const { return S.stride (axis); }

        value_type value () { return get_voxel()[S[3]]; }

        void get_values (value_type* buffer) { 
          if (S[0] < 0 || S[0] >= dim(0) ||
              S[1] < 0 || S[1] >= dim(1) ||
              S[2] < 0 || S[2] >= dim(2)) 
            memset (buffer, 0, dim(3)*sizeof(value_type));
          else
            memcpy (buffer, get_voxel(), dim(3)*sizeof(value_type)); 
        }

        Image::Position<Bootstrap<Set,Functor,NUM_VOX_PER_CHUNK> > operator[] (size_t axis )
        {
          return Image::Position<Bootstrap<Set,Functor,NUM_VOX_PER_CHUNK> > (*this, axis);
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
        Set& S;
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
          value_type*& data (voxels.insert (std::make_pair (Point<ssize_t> (S[0], S[1], S[2]), (float*)NULL)).first->second);
          if (!data) {
            data = allocate_voxel ();
            ssize_t pos = S[3];
            for (S[3] = 0; S[3] < S.dim(3); ++S[3])
              data[S[3]] = S.value();
            S[3] = pos;
            func (data);
          }
          return data;
        }

        ssize_t get_pos (size_t axis) const { return S[axis]; }
        void set_pos (size_t axis, ssize_t position) const { S[axis] = position; }
        void move_pos (size_t axis, ssize_t increment) const { S[axis] += increment; }

        friend class Image::Position<Bootstrap<Set,Functor,NUM_VOX_PER_CHUNK> >;
    };

  }
}

#endif




